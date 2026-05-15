#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include "PvpRealtimeSocket.hpp"

#include <string>

using namespace geode::prelude;

class PvpOverlay final : public PvpRealtimeSocketDelegate {
public:
	explicit PvpOverlay(PlayLayer* layer, int levelID);
	~PvpOverlay() override;

	void update(float dt);
	void cleanup();

	void onRealtimeOpen() override;
	void onRealtimeMessage(std::string const& message) override;
	void onRealtimeClose() override;

private:
	struct PlayerProgress {
		std::string uid;
		float progress = 0.0f;
	};

	PlayLayer* m_layer = nullptr;
	CCLabelBMFont* m_label = nullptr;
	std::shared_ptr<PvpRealtimeSocket> m_socket;
	async::TaskHolder<web::WebResponse> m_matchHolder;
	async::TaskHolder<web::WebResponse> m_tokenHolder;

	int m_levelID = 0;
	int m_matchID = 0;
	int m_ref = 1;
	int m_reconnectAttempts = 0;
	float m_heartbeatTimer = 0.0f;
	float m_reconnectTimer = -1.0f;
	bool m_active = false;
	bool m_cleanedUp = false;
	bool m_connecting = false;
	bool m_joined = false;

	std::string m_currentUid;
	std::string m_supabaseUrl;
	std::string m_anonKey;
	std::string m_topic;
	PlayerProgress m_self;
	PlayerProgress m_opponent;

	void createLabel();
	void requestMatch();
	void requestRealtimeToken();
	void connectRealtime();
	void closeSocket();
	void scheduleReconnect();
	void sendJoin();
	void sendHeartbeat();
	void sendJson(matjson::Value const& json);
	void handleRealtimeMessage(matjson::Value const& json);
	void handleResultRow(matjson::Value const& row);
	void handleMatchRow(matjson::Value const& row);
	void parseMatchSnapshot(matjson::Value const& json);
	void refreshLabel();
	void updateLabelPosition();
	void setOverlayVisible(bool visible);
	bool isReadyForRealtime() const;
	bool isActiveStatus(std::string const& status) const;
	std::string nextRef();
};
