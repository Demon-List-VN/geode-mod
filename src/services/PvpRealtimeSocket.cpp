#include "PvpRealtimeSocket.hpp"

#ifdef __ANDROID__

#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct libwebsocket;
struct libwebsocket_context;
struct libwebsocket_protocols;

#ifndef __CC_WEBSOCKET_H__
namespace cocos2d::extension {
class WsThreadHelper;
class WsMessage;

class WebSocket {
public:
	WebSocket();
	virtual ~WebSocket();

	struct Data {
		Data() : bytes(nullptr), len(0), isBinary(false) {}
		char* bytes;
		int len;
		bool isBinary;
	};

	enum ErrorCode {
		kErrorTimeout = 0,
		kErrorConnectionFailure,
		kErrorUnknown
	};

	class Delegate {
	public:
		virtual ~Delegate() {}
		virtual void onOpen(WebSocket* ws) = 0;
		virtual void onMessage(WebSocket* ws, Data const& data) = 0;
		virtual void onClose(WebSocket* ws) = 0;
		virtual void onError(WebSocket* ws, ErrorCode const& error) = 0;
	};

	bool init(Delegate const& delegate, gd::string const& url, gd::vector<gd::string> const* protocols = nullptr);
	void send(gd::string const& message);
	void send(unsigned char const* binaryMsg, unsigned int len);
	void close();

	enum State {
		kStateConnecting = 0,
		kStateOpen,
		kStateClosing,
		kStateClosed
	};

	State getReadyState();

private:
	virtual void onSubThreadStarted();
	virtual int onSubThreadLoop();
	virtual void onSubThreadEnded();
	virtual void onUIThreadReceiveMessage(WsMessage* msg);

public:
	State _readyState;
	gd::string _host;
	unsigned int _port;
	gd::string _path;
	WsThreadHelper* _wsHelper;
	libwebsocket* _wsInstance;
	libwebsocket_context* _wsContext;
	Delegate* _delegate;
	int _SSLConnection;
	libwebsocket_protocols* _wsProtocols;
};
}
#endif

namespace {
class CocosPvpRealtimeSocket final
	: public PvpRealtimeSocket,
	  public cocos2d::extension::WebSocket::Delegate,
	  public std::enable_shared_from_this<CocosPvpRealtimeSocket> {
public:
	explicit CocosPvpRealtimeSocket(PvpRealtimeSocketDelegate* delegate) : m_delegate(delegate) {}

	~CocosPvpRealtimeSocket() override {
		this->close();
	}

	bool connect(std::string const& url) override {
		if (m_socket || m_closed) {
			return false;
		}

		auto socket = std::make_unique<cocos2d::extension::WebSocket>();
		if (!socket->init(*this, gd::string(url))) {
			return false;
		}

		m_socket = std::move(socket);
		return true;
	}

	void send(std::string const& message) override {
		if (!this->isOpen()) {
			return;
		}

		m_socket->send(gd::string(message));
	}

	void close() override {
		if (m_closed) {
			return;
		}

		m_closed = true;
		m_open = false;
		m_delegate = nullptr;

		if (m_socket) {
			m_socket->close();
			m_socket.reset();
		}
	}

	bool isOpen() const override {
		return m_socket && m_open && !m_closed && m_socket->getReadyState() == cocos2d::extension::WebSocket::kStateOpen;
	}

	void onOpen(cocos2d::extension::WebSocket*) override {
		m_open = true;

		auto weakSelf = this->weak_from_this();
		Loader::get()->queueInMainThread([weakSelf] {
			if (auto self = weakSelf.lock(); self && self->m_delegate && !self->m_closed) {
				self->m_delegate->onRealtimeOpen();
			}
		});
	}

	void onMessage(cocos2d::extension::WebSocket*, cocos2d::extension::WebSocket::Data const& data) override {
		if (!m_delegate || m_closed || data.isBinary || !data.bytes || data.len <= 0) {
			return;
		}

		auto message = std::string(data.bytes, static_cast<size_t>(data.len));
		auto weakSelf = this->weak_from_this();

		Loader::get()->queueInMainThread([weakSelf, message = std::move(message)] {
			if (auto self = weakSelf.lock(); self && self->m_delegate && !self->m_closed) {
				self->m_delegate->onRealtimeMessage(message);
			}
		});
	}

	void onClose(cocos2d::extension::WebSocket*) override {
		this->finishClosed();
	}

	void onError(cocos2d::extension::WebSocket*, cocos2d::extension::WebSocket::ErrorCode const&) override {
		this->finishClosed();
	}

private:
	PvpRealtimeSocketDelegate* m_delegate = nullptr;
	std::unique_ptr<cocos2d::extension::WebSocket> m_socket;
	bool m_open = false;
	bool m_closed = false;

	void finishClosed() {
		if (m_closed) {
			return;
		}

		m_open = false;
		m_closed = true;

		auto weakSelf = this->weak_from_this();
		Loader::get()->queueInMainThread([weakSelf] {
			if (auto self = weakSelf.lock(); self && self->m_delegate) {
				auto* delegate = self->m_delegate;
				self->m_delegate = nullptr;
				delegate->onRealtimeClose();
			}
		});
	}
};
}

std::shared_ptr<PvpRealtimeSocket> PvpRealtimeSocket::create(PvpRealtimeSocketDelegate* delegate) {
	return std::make_shared<CocosPvpRealtimeSocket>(delegate);
}

#elif !defined(__APPLE__)

namespace {
class StubPvpRealtimeSocket final : public PvpRealtimeSocket {
public:
	explicit StubPvpRealtimeSocket(PvpRealtimeSocketDelegate*) {}

	bool connect(std::string const&) override {
		return false;
	}

	void send(std::string const&) override {}
	void close() override {}

	bool isOpen() const override {
		return false;
	}
};
}

std::shared_ptr<PvpRealtimeSocket> PvpRealtimeSocket::create(PvpRealtimeSocketDelegate* delegate) {
	return std::make_shared<StubPvpRealtimeSocket>(delegate);
}

#endif
