#include "WebsocketClient.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXSocketTLSOptions.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketMessageType.h>

#include <Geode/loader/Loader.hpp>

#include <atomic>
#include <mutex>
#include <utility>

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
    options.caFile = "NONE";
#else
    options.caFile = "SYSTEM";
#endif

    return options;
}
} // namespace

WebsocketClient::WebsocketClient() = default;

WebsocketClient::~WebsocketClient() {
    this->close();
}

bool WebsocketClient::connectSocket(std::string url, Callbacks callbacks) {
    if (m_socket || m_closed.load() || url.empty()) {
        return false;
    }

    ensureIxNetSystem();
    m_userClosed.store(false);

    {
        std::lock_guard lock(m_callbackMutex);
        m_callbacks = std::move(callbacks);
    }

    auto socket = std::make_unique<ix::WebSocket>();
    socket->setUrl(std::move(url));
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
            self->handleSocketOpen();
            break;

        case ix::WebSocketMessageType::Message:
            if (!msg->binary) {
                self->handleSocketMessage(msg->str);
            }
            break;

        case ix::WebSocketMessageType::Close:
        case ix::WebSocketMessageType::Error:
            self->handleSocketClosed();
            break;

        default:
            break;
        }
    });

    m_socket = std::move(socket);
    m_socket->start();
    return true;
}

void WebsocketClient::update(float) {
}

void WebsocketClient::close() {
    m_userClosed.store(true);
    m_open.store(false);
    this->clearCallbacks();

    m_closed.store(true);

    if (m_socket) {
        m_socket->stop();
        m_socket.reset();
    }
}

bool WebsocketClient::isOpen() const {
    return m_socket && m_open.load() && !m_closed.load() && m_socket->getReadyState() == ix::ReadyState::Open;
}

bool WebsocketClient::isClosed() const {
    return m_closed.load();
}

bool WebsocketClient::hasSocket() const {
    return m_socket != nullptr;
}

void WebsocketClient::sendText(std::string const& text) {
    if (!this->isOpen() || !m_socket) {
        return;
    }

    m_socket->sendUtf8Text(text);
}

void WebsocketClient::handleSocketOpen() {
    if (m_closed.load() || m_userClosed.load()) {
        return;
    }

    m_open.store(true);
    auto callback = this->callbacks().onOpen;
    auto weakSelf = this->weak_from_this();

    geode::Loader::get()->queueInMainThread([weakSelf, callback] {
        auto self = weakSelf.lock();
        if (self && !self->m_closed.load() && !self->m_userClosed.load() && callback) {
            callback();
        }
    });
}

void WebsocketClient::handleSocketMessage(std::string const& message) {
    if (m_closed.load() || m_userClosed.load()) {
        return;
    }

    auto callback = this->callbacks().onMessage;
    auto weakSelf = this->weak_from_this();

    geode::Loader::get()->queueInMainThread([weakSelf, callback, message] {
        auto self = weakSelf.lock();
        if (self && !self->m_closed.load() && !self->m_userClosed.load() && callback) {
            callback(message);
        }
    });
}

void WebsocketClient::handleSocketClosed() {
    if (m_closed.exchange(true)) {
        return;
    }

    m_open.store(false);
    auto callback = this->callbacks().onClose;
    this->clearCallbacks();
    auto weakSelf = this->weak_from_this();

    geode::Loader::get()->queueInMainThread([weakSelf, callback] {
        auto self = weakSelf.lock();
        if (self && !self->m_userClosed.load() && callback) {
            callback();
        }
    });
}

WebsocketClient::Callbacks WebsocketClient::callbacks() const {
    std::lock_guard lock(m_callbackMutex);
    return m_callbacks;
}

void WebsocketClient::clearCallbacks() {
    std::lock_guard lock(m_callbackMutex);
    m_callbacks = {};
}
