#pragma once

#include "../../clients/pvp/PvpWebsocketClient.hpp"
#include "../../dtos/pvp/PvpMessageDto.hpp"
#include "../../dtos/pvp/PvpMessagesResponseDto.hpp"
#include "../../dtos/pvp/match/PvpMatchPlayerProgressDto.hpp"
#include "../../dtos/pvp/match/PvpMatchRealtimeMessageDto.hpp"
#include "../../dtos/pvp/match/PvpMatchRowDto.hpp"
#include "../../dtos/pvp/match/PvpMatchSnapshotDto.hpp"
#include "../../dtos/pvp/match/PvpMatchSystemMetadataDto.hpp"
#include "../../models/pvp/overlay/PvpOverlayChatMessageModel.hpp"
#include "../../models/pvp/overlay/PvpOverlayPlayerProgressModel.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace geode::prelude;

class PvpSubmitterService;
class PvpChatPopup;
class PvpOverlay;
class PvpRecentChatStack;

class PvpOverlayService final {
  public:
    explicit PvpOverlayService(PlayLayer* layer, int levelID, PvpSubmitterService* submitter = nullptr);
    ~PvpOverlayService();

    static PvpOverlayService* getActive();

    void update(float dt);
    void cleanup();
    bool openChat();
    bool hasPvpMatch() const;
    bool isChatUsable() const;
    bool isChatMuted() const;
    void setChatMuted(bool muted);
    void submitChatMessage(std::string content);
    void notifyChatPopupClosed(PvpChatPopup* popup);
    std::string getChatHistoryText() const;
    std::vector<std::string> getChatHistoryLines() const;

  private:
    static constexpr float CHAT_GRACE_SECONDS = 3.0f * 60.0f;
    static constexpr float MESSAGE_REFRESH_COALESCE = 0.2f;
    static constexpr int MAX_CHAT_MESSAGE_LENGTH = 500;
    static constexpr int MESSAGE_FETCH_LIMIT = 100;
    static PvpOverlayService* s_activeOverlay;

    PlayLayer* m_layer = nullptr;
    std::unique_ptr<PvpOverlay> m_overlay;
    std::unique_ptr<PvpRecentChatStack> m_recentChatStack;
    PvpChatPopup* m_chatPopup = nullptr;
    PvpSubmitterService* m_submitter = nullptr;
    std::shared_ptr<PvpWebsocketClient> m_websocketClient;

    int m_levelID = 0;
    int m_matchID = 0;
    int m_reconnectAttempts = 0;
    float m_reconnectTimer = -1.0f;
    float m_messageRefreshTimer = -1.0f;
    float m_chatGraceTimer = -1.0f;
    bool m_active = false;
    bool m_chatOpen = false;
    bool m_chatMuted = false;
    bool m_chatSending = false;
    bool m_cleanedUp = false;
    bool m_connecting = false;
    bool m_requestingRealtimeToken = false;
    bool m_hideOverlayForLevelChange = false;

    std::int64_t m_latestMessageID = 0;
    std::int64_t m_realtimeTokenExpiresAt = 0;
    std::int64_t m_matchEndsAtEpoch = 0;
    std::int64_t m_lastCountdownSeconds = -1;
    std::string m_currentUid;
    std::string m_supabaseUrl;
    std::string m_anonKey;
    std::string m_realtimeAccessToken;
    std::string m_mode = "classic";
    std::string m_scoringMode = "progress";
    int m_targetScore = 0;
    std::string m_context = "versus";
    std::string m_roomName;
    PvpOverlayPlayerProgressModel m_self;
    PvpOverlayPlayerProgressModel m_opponent;
    std::vector<PvpOverlayPlayerProgressModel> m_players;
    std::vector<PvpOverlayChatMessageModel> m_chatMessages;

    void requestMatch();
    void requestRealtimeToken();
    void requestMessages(bool animateNew, bool incremental);
    void connectRealtime();
    void closeRealtime();
    void scheduleReconnect();
    void handleRealtimeOpen();
    void handleRealtimeMessage(PvpMatchRealtimeMessageDto const& message);
    void handleRealtimeClose();
    void scheduleMessageRefresh();
    void handleResultRow(PvpMatchPlayerProgressDto const& row);
    void handleMatchRow(PvpMatchRowDto const& row);
    void handleMessagesPayload(PvpMessagesResponseDto const& messages, bool animateNew);
    void handleMessageRow(PvpMessageDto const& message, bool animateNew);
    void handleSystemMetadata(PvpMatchSystemMetadataDto const& metadata);
    void parseMatchSnapshot(PvpMatchSnapshotDto const& snapshot);
    std::string formatSystemMessage(PvpMatchSystemMetadataDto const& metadata) const;
    std::string formatPlayerLabel(std::string const& label, PvpOverlayPlayerProgressModel const& player) const;
    std::string participantLabel(std::string const& uid) const;
    std::string getChatSenderLabel(PvpOverlayChatMessageModel const& message) const;
    std::vector<PvpOverlayPlayerProgressModel> sortedPlayers() const;
    void pushRecentMessage(PvpOverlayChatMessageModel const& message);
    void updateRecentMessages(float dt);
    void refreshLabel();
    void refreshChatVisibility();
    void setOverlayVisible(bool visible);
    bool isReadyForRealtime() const;
    bool isActiveStatus(std::string const& status) const;
    bool isCompletedStatus(std::string const& status) const;
    bool isPlatformerMode() const;
    bool isCustomRoomMatch() const;
    std::string formatProgressLabel(float progress) const;
};
