#pragma once

#include "../../dtos/pvp/match/PvpMatchRealtimeMessageDto.hpp"
#include "../websocket/WebsocketClient.hpp"

#include <functional>
#include <memory>
#include <string>

class PvpWebsocketClient final : public WebsocketClient {
  public:
    using OpenCallback = std::function<void()>;
    using MessageCallback = std::function<void(PvpMatchRealtimeMessageDto const&)>;
    using CloseCallback = std::function<void()>;

    struct Callbacks {
        OpenCallback onOpen;
        MessageCallback onMessage;
        CloseCallback onClose;
    };

    explicit PvpWebsocketClient(Callbacks callbacks);
    ~PvpWebsocketClient() override;

    static std::shared_ptr<PvpWebsocketClient> create(Callbacks callbacks);

    bool connect(std::string supabaseUrl, std::string anonKey, int matchID, std::string accessToken);
    void update(float dt) override;
    void close() override;

  private:
    static constexpr float HEARTBEAT_INTERVAL = 25.0f;

    Callbacks m_callbacks;
    int m_matchID = 0;
    int m_ref = 1;
    float m_heartbeatTimer = 0.0f;
    bool m_joined = false;
    std::string m_accessToken;
    std::string m_topic;

    void handleOpen();
    void handleMessage(std::string const& message);
    void handleJoinReply(PvpMatchRealtimeMessageDto const& message);
    void handleClosed();
    void sendJoin();
    void sendHeartbeat();
    void sendJson(matjson::Value const& json);
    std::string nextRef();
    void clearCallbacks();
};
