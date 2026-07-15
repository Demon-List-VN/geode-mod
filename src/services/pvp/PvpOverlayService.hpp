#pragma once

#include "../../clients/pvp/PvpWebsocketClient.hpp"
#include "../../dtos/pvp/PvpMessageDto.hpp"
#include "../../dtos/pvp/PvpMessagesResponseDto.hpp"
#include "../../dtos/pvp/PvpPowerupCastResponseDto.hpp"
#include "../../dtos/pvp/PvpPowerupStateDto.hpp"
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
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

class PvpSubmitterService;
class PvpChatPopup;
class PvpPowerupPopup;
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
    bool openPowerups();
    bool isPowerupMode() const;
    std::string currentUid() const;
    std::vector<PvpOverlayPlayerProgressModel> powerupTargets() const;
    void requestPowerupState(std::function<void(PvpPowerupStateDto const&, bool)> callback);
    void castPowerupSkill(std::string const& skill,
                          std::string const& targetUid,
                          bool randomTarget,
                          std::function<void(PvpPowerupCastResponseDto const&, bool)> callback);
    bool shouldBlockButtonDown(int button, bool isPlayer1);
    bool shouldBlockButtonRelease(int button, bool isPlayer1);
    void registerForceResetClick();
    void notifyChatPopupClosed(PvpChatPopup* popup);
    void notifyPowerupPopupClosed(PvpPowerupPopup* popup);
    std::string getChatHistoryText() const;
    std::vector<std::string> getChatHistoryLines() const;

  private:
    struct PendingRevealMessage {
        PvpMessageDto message;
        std::int64_t revealAtEpoch = 0;
        bool animateNew = false;
    };

    static constexpr float CHAT_GRACE_SECONDS = 3.0f * 60.0f;
    static constexpr float MESSAGE_REFRESH_COALESCE = 0.2f;
    static constexpr float POWERUP_STATE_REFRESH_COALESCE = 0.25f;
    static constexpr int MAX_CHAT_MESSAGE_LENGTH = 500;
    static constexpr int MESSAGE_FETCH_LIMIT = 100;
    static PvpOverlayService* s_activeOverlay;

    PlayLayer* m_layer = nullptr;
    std::unique_ptr<PvpOverlay> m_overlay;
    std::unique_ptr<PvpRecentChatStack> m_recentChatStack;
    PvpChatPopup* m_chatPopup = nullptr;
    PvpPowerupPopup* m_powerupPopup = nullptr;
    PvpSubmitterService* m_submitter = nullptr;
    std::shared_ptr<PvpWebsocketClient> m_websocketClient;

    int m_levelID = 0;
    int m_matchID = 0;
    int m_reconnectAttempts = 0;
    float m_reconnectTimer = -1.0f;
    float m_messageRefreshTimer = -1.0f;
    float m_powerupStateRefreshTimer = -1.0f;
    float m_chatGraceTimer = -1.0f;
    bool m_active = false;
    bool m_chatOpen = false;
    bool m_chatMuted = false;
    bool m_chatSending = false;
    bool m_powerupStateLoaded = false;
    bool m_powerupStateRequesting = false;
    bool m_cleanedUp = false;
    bool m_connecting = false;
    bool m_requestingRealtimeToken = false;
    bool m_hideOverlayForLevelChange = false;
    bool m_invisibleActive = false;
    bool m_player1WasVisible = true;
    bool m_player2WasVisible = true;
    float m_flashbangTimer = -1.0f;
    float m_invisibleTimer = -1.0f;
    float m_doubleClickTimer = -1.0f;
    float m_forceResetTimer = -1.0f;
    CCLayerColor* m_flashbangOverlay = nullptr;

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
    int m_startingHp = 0;
    int m_finalizeAliveCount = 0;
    std::string m_context = "versus";
    std::string m_roomName;
    PvpPowerupStateDto m_powerupState;
    PvpOverlayPlayerProgressModel m_self;
    PvpOverlayPlayerProgressModel m_opponent;
    std::vector<PvpOverlayPlayerProgressModel> m_players;
    std::vector<PvpOverlayChatMessageModel> m_chatMessages;
    std::vector<PendingRevealMessage> m_pendingRevealMessages;
    std::unordered_set<int> m_doubleClickWaitingButtons;
    std::unordered_set<int> m_doubleClickBlockedButtons;
    int m_forceResetClicks = 0;
    int m_forceResetRequiredClicks = 10;

    void requestMatch();
    void requestRealtimeToken();
    void requestMessages(bool animateNew, bool incremental);
    void refreshPowerupState();
    void schedulePowerupStateRefresh();
    void applyPowerupState(PvpPowerupStateDto const& state);
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
    void startPlayerProgressRun(std::string const& uid, float progress, float progressSpeed,
                                std::int64_t sampledAtMs = 0);
    void stopPlayerProgressRun(std::string const& uid, float progress);
    bool updatePlayerProgressRuns();
    void parseMatchSnapshot(PvpMatchSnapshotDto const& snapshot);
    std::string formatSystemMessage(PvpMatchSystemMetadataDto const& metadata) const;
    std::string formatPlayerLabel(std::string const& label, PvpOverlayPlayerProgressModel const& player) const;
    std::string formatPlayerMana(std::string const& uid) const;
    std::string participantLabel(std::string const& uid) const;
    std::string getChatSenderLabel(PvpOverlayChatMessageModel const& message) const;
    std::vector<PvpOverlayPlayerProgressModel> sortedPlayers() const;
    void pushRecentMessage(PvpOverlayChatMessageModel const& message);
    void updateRecentMessages(float dt);
    void processPendingRevealMessages();
    void handlePowerupSkill(PvpMatchSystemMetadataDto const& metadata);
    void startFlashbang(float durationSeconds);
    void clearFlashbang();
    void startInvisible(float durationSeconds);
    void clearInvisible();
    void startDoubleClick(float durationSeconds);
    void clearDoubleClick();
    void startForceResetChallenge(PvpMatchSystemMetadataDto const& metadata, float durationSeconds);
    void clearForceResetChallenge();
    void openPauseLayer();
    void forceReset();
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
