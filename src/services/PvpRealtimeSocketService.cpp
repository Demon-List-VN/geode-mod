#include "PvpRealtimeSocketService.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXSocketTLSOptions.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketMessageType.h>

#include <Geode/loader/Loader.hpp>

#include <atomic>

namespace {
void ensureIxNetSystem() {
	static std::atomic_bool initialized = false;
	bool expected = false;
	if (initialized.compare_exchange_strong(expected, true)) {
		ix::initNetSystem();
	}
}

ix::SocketTLSOptions tlsOptions() {
	ix::SocketTLSOptions options;

#ifdef __ANDROID__
	// IXWebSocket's mbedTLS backend cannot read Android's system certificate store.
	// HTTPS API calls still use Geode/libcurl verification; this only affects the
	// Supabase realtime websocket on Android.
	options.caFile = "NONE";
#else
	options.caFile = "SYSTEM";
#endif

	return options;
}

class IxPvpRealtimeSocketService final : public PvpRealtimeSocketService, public std::enable_shared_from_this<IxPvpRealtimeSocketService> {
public:
	explicit IxPvpRealtimeSocketService(PvpRealtimeSocketDelegateService* delegate) : m_delegate(delegate) {}

	~IxPvpRealtimeSocketService() override {
		this->close();
	}

	bool connect(std::string const& url) override {
		if (m_socket || m_closed.load()) {
			return false;
		}

		ensureIxNetSystem();

		auto socket = std::make_unique<ix::WebSocket>();
		socket->setUrl(url);
		socket->setTLSOptions(tlsOptions());
		socket->setHandshakeTimeout(10);
		socket->disableAutomaticReconnection();
		socket->disablePerMessageDeflate();

		auto weakSelf = this->weak_from_this();
		socket->setOnMessageCallback([weakSelf](ix::WebSocketMessagePtr const& msg) {
			auto self = weakSelf.lock();
			if (!self || !msg) {
				return;
			}

			switch (msg->type) {
				case ix::WebSocketMessageType::Open:
					self->handleOpen();
					break;

				case ix::WebSocketMessageType::Message:
					if (!msg->binary) {
						self->handleMessage(msg->str);
					}
					break;

				case ix::WebSocketMessageType::Close:
				case ix::WebSocketMessageType::Error:
					self->handleClosed();
					break;

				default:
					break;
			}
		});

		m_socket = std::move(socket);
		m_socket->start();
		return true;
	}

	void send(std::string const& message) override {
		if (!this->isOpen() || !m_socket) {
			return;
		}

		m_socket->sendUtf8Text(message);
	}

	void close() override {
		if (m_closed.exchange(true)) {
			return;
		}

		m_open.store(false);
		m_delegate.store(nullptr);

		if (m_socket) {
			m_socket->stop();
			m_socket.reset();
		}
	}

	bool isOpen() const override {
		return m_socket && m_open.load() && !m_closed.load() && m_socket->getReadyState() == ix::ReadyState::Open;
	}

private:
	std::atomic<PvpRealtimeSocketDelegateService*> m_delegate = nullptr;
	std::unique_ptr<ix::WebSocket> m_socket;
	std::atomic_bool m_open = false;
	std::atomic_bool m_closed = false;

	void handleOpen() {
		if (m_closed.load()) {
			return;
		}

		m_open.store(true);
		auto weakSelf = this->weak_from_this();

		geode::Loader::get()->queueInMainThread([weakSelf] {
			if (auto self = weakSelf.lock()) {
				if (auto* delegate = self->m_delegate.load(); delegate && !self->m_closed.load()) {
					delegate->onRealtimeOpen();
				}
			}
		});
	}

	void handleMessage(std::string const& message) {
		if (m_closed.load()) {
			return;
		}

		auto weakSelf = this->weak_from_this();

		geode::Loader::get()->queueInMainThread([weakSelf, message] {
			if (auto self = weakSelf.lock()) {
				if (auto* delegate = self->m_delegate.load(); delegate && !self->m_closed.load()) {
					delegate->onRealtimeMessage(message);
				}
			}
		});
	}

	void handleClosed() {
		if (m_closed.exchange(true)) {
			return;
		}

		m_open.store(false);
		auto weakSelf = this->weak_from_this();

		geode::Loader::get()->queueInMainThread([weakSelf] {
			if (auto self = weakSelf.lock()) {
				if (auto* delegate = self->m_delegate.exchange(nullptr)) {
					delegate->onRealtimeClose();
				}
			}
		});
	}
};
}

std::shared_ptr<PvpRealtimeSocketService> PvpRealtimeSocketService::create(PvpRealtimeSocketDelegateService* delegate) {
	return std::make_shared<IxPvpRealtimeSocketService>(delegate);
}
