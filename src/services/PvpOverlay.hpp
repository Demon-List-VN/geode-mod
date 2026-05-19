#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include "PvpRealtimeSocket.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace geode::prelude;

class PvpChatPopup;

class PvpOverlay final : public PvpRealtimeSocketDelegate {
public:
	explicit PvpOverlay(PlayLayer* layer, int levelID);
	~PvpOverlay() override;

	static PvpOverlay* getActive();

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

	void onRealtimeOpen() override;
	void onRealtimeMessage(std::string const& message) override;
	void onRealtimeClose() override;

private:
	struct PlayerProgress {
		std::string uid;
		float progress = 0.0f;
	};

	struct ChatMessage {
		std::int64_t id = 0;
		std::string senderUid;
		std::string type;
		std::string content;
		bool senderAnonymous = false;
	};

	struct RecentChatMessage {
		std::int64_t id = 0;
		float timeLeft = 0.0f;
		CCLabelBMFont* label = nullptr;
	};

	PlayLayer* m_layer = nullptr;
	CCLabelBMFont* m_label = nullptr;
	CCNode* m_chatStack = nullptr;
	PvpChatPopup* m_chatPopup = nullptr;
	std::shared_ptr<PvpRealtimeSocket> m_socket;
	async::TaskHolder<web::WebResponse> m_matchHolder;
	async::TaskHolder<web::WebResponse> m_tokenHolder;
	async::TaskHolder<web::WebResponse> m_messagesHolder;
	async::TaskHolder<web::WebResponse> m_sendMessageHolder;

	int m_levelID = 0;
	int m_matchID = 0;
	int m_ref = 1;
	int m_reconnectAttempts = 0;
	float m_heartbeatTimer = 0.0f;
	float m_reconnectTimer = -1.0f;
	float m_messageRefreshTimer = -1.0f;
	float m_chatGraceTimer = -1.0f;
	bool m_active = false;
	bool m_chatOpen = false;
	bool m_chatMuted = false;
	bool m_chatSending = false;
	bool m_cleanedUp = false;
	bool m_connecting = false;
	bool m_joined = false;
	bool m_requestingRealtimeToken = false;

	std::int64_t m_latestMessageID = 0;
	std::int64_t m_realtimeTokenExpiresAt = 0;
	std::string m_currentUid;
	std::string m_supabaseUrl;
	std::string m_anonKey;
	std::string m_realtimeAccessToken;
	std::string m_topic;
	std::string m_mode = "classic";
	PlayerProgress m_self;
	PlayerProgress m_opponent;
	std::vector<ChatMessage> m_chatMessages;
	std::vector<RecentChatMessage> m_recentMessages;

	void createLabel();
	void createChatNodes();
	void requestMatch();
	void requestRealtimeToken();
	void requestMessages(bool animateNew, bool incremental);
	void connectRealtime();
	void closeSocket();
	void scheduleReconnect();
	void scheduleMessageRefresh();
	void sendJoin();
	void sendHeartbeat();
	void sendJson(matjson::Value const& json);
	void handleRealtimeMessage(matjson::Value const& json);
	void handleResultRow(matjson::Value const& row);
	void handleMatchRow(matjson::Value const& row);
	void handleMessagesPayload(matjson::Value const& json, bool animateNew);
	void handleMessageRow(matjson::Value const& row, bool animateNew);
	void parseMatchSnapshot(matjson::Value const& json);
	std::string formatSystemMessage(matjson::Value const& metadata) const;
	std::string getChatSenderLabel(ChatMessage const& message) const;
	void pushRecentMessage(ChatMessage const& message);
	void layoutRecentMessages();
	void updateRecentMessages(float dt);
	void refreshLabel();
	void refreshChatVisibility();
	void updateLabelPosition();
	void updateChatPositions();
	void setOverlayVisible(bool visible);
	bool isReadyForRealtime() const;
	bool isActiveStatus(std::string const& status) const;
	bool isCompletedStatus(std::string const& status) const;
	bool isPlatformerMode() const;
	std::string formatProgressLabel(float progress) const;
	std::string nextRef();
};
