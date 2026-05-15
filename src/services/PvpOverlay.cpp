#include "PvpOverlay.hpp"

#include "AuthService.hpp"
#include "../common.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace {
constexpr float HEARTBEAT_INTERVAL = 25.0f;
constexpr float LABEL_MARGIN = 8.0f;

std::string trimTrailingSlash(std::string value) {
	while (!value.empty() && value.back() == '/') {
		value.pop_back();
	}
	return value;
}

std::string realtimeUrl(std::string url, std::string const& anonKey) {
	url = trimTrailingSlash(std::move(url));

	if (url.rfind("https://", 0) == 0) {
		url.replace(0, 5, "wss");
	} else if (url.rfind("http://", 0) == 0) {
		url.replace(0, 4, "ws");
	}

	return url + "/realtime/v1/websocket?apikey=" + anonKey + "&vsn=1.0.0";
}

std::string getString(matjson::Value const& json, char const* key) {
	if (!json[key].isString()) {
		return "";
	}

	return json[key].asString().unwrapOrDefault();
}

float getNumber(matjson::Value const& json, char const* key) {
	if (!json[key].isNumber()) {
		return 0.0f;
	}

	return static_cast<float>(json[key].asDouble().unwrapOr(0.0));
}

matjson::Value makeChange(std::string const& event, std::string const& table, std::string const& filter) {
	auto change = matjson::Value::object();
	change["event"] = event;
	change["schema"] = "public";
	change["table"] = table;
	change["filter"] = filter;
	return change;
}

matjson::Value const& realtimeRecord(matjson::Value const& payload) {
	auto const& data = payload["data"];

	if (data["record"].isObject()) {
		return data["record"];
	}

	if (payload["new"].isObject()) {
		return payload["new"];
	}

	return payload;
}
}

PvpOverlay::PvpOverlay(PlayLayer* layer, int levelID) : m_layer(layer), m_levelID(levelID) {
	this->createLabel();
	this->requestMatch();
}

PvpOverlay::~PvpOverlay() {
	this->cleanup();
}

void PvpOverlay::createLabel() {
	if (!m_layer) {
		return;
	}

	m_label = CCLabelBMFont::create("PvP\nYou: 0.00%\nOpponent: 0.00%", "bigFont.fnt");
	m_label->setScale(0.32f);
	m_label->setOpacity(180);
	m_label->setVisible(false);
	m_label->setID("gdvn-pvp-overlay"_spr);

	auto parent = m_layer->m_uiLayer ? static_cast<CCNode*>(m_layer->m_uiLayer) : static_cast<CCNode*>(m_layer);
	parent->addChild(m_label, 1000);
	this->updateLabelPosition();
}

void PvpOverlay::requestMatch() {
	if (!AuthService::isLoggedIn() || m_cleanedUp) {
		return;
	}

	web::WebRequest req;
	req.header("Authorization", "Bearer " + AuthService::getToken());
	std::string url = API_URL + "/levels/" + std::to_string(m_levelID) + "/inPvp";

	m_matchHolder.spawn(req.get(url), [this](web::WebResponse res) {
		if (m_cleanedUp) {
			return;
		}

		if (!res.ok()) {
			this->setOverlayVisible(false);
			return;
		}

		try {
			auto json = res.json().unwrap();
			this->parseMatchSnapshot(json);
			this->refreshLabel();

			if (this->isReadyForRealtime()) {
				this->requestRealtimeToken();
			}
		} catch (...) {
			log::warn("Failed to parse PvP overlay match snapshot");
		}
	});
}

void PvpOverlay::parseMatchSnapshot(matjson::Value const& json) {
	m_matchID = static_cast<int>(getNumber(json, "matchId"));
	m_currentUid = getString(json, "currentUid");
	m_active = this->isActiveStatus(getString(json, "status"));

	m_self = {};
	m_opponent = {};

	if (json["participants"].isArray()) {
		for (auto const& participant : json["participants"].asArray().unwrap()) {
			PlayerProgress player;
			player.uid = getString(participant, "uid");
			player.progress = getNumber(participant, "progress");

			if (!participant["result"].isNull() && participant["result"].isObject()) {
				player.progress = std::max(player.progress, getNumber(participant["result"], "progress"));
			}

			if (!m_currentUid.empty() && player.uid == m_currentUid) {
				m_self = player;
			} else if (m_opponent.uid.empty()) {
				m_opponent = player;
			}
		}
	}

	if (json["results"].isArray()) {
		for (auto const& result : json["results"].asArray().unwrap()) {
			this->handleResultRow(result);
		}
	}

	this->setOverlayVisible(m_active);
}

void PvpOverlay::requestRealtimeToken() {
	if (m_cleanedUp) {
		return;
	}

	web::WebRequest req;
	req.header("Authorization", "Bearer " + AuthService::getToken());
	std::string url = API_URL + "/auth/realtime-token";

	m_tokenHolder.spawn(req.get(url), [this](web::WebResponse res) {
		if (m_cleanedUp) {
			return;
		}

		if (!res.ok()) {
			log::warn("Failed to get PvP realtime token: HTTP {}", res.code());
			return;
		}

		try {
			auto json = res.json().unwrap();
			m_supabaseUrl = getString(json, "supabaseUrl");
			m_anonKey = getString(json, "anonKey");
			this->connectRealtime();
		} catch (...) {
			log::warn("Failed to parse PvP realtime token");
		}
	});
}

bool PvpOverlay::isReadyForRealtime() const {
	return m_matchID > 0 && m_active && !m_cleanedUp;
}

void PvpOverlay::connectRealtime() {
	if (!this->isReadyForRealtime() || m_supabaseUrl.empty() || m_anonKey.empty() || m_socket) {
		return;
	}

	m_connecting = true;
	m_joined = false;
	m_heartbeatTimer = 0.0f;
	m_topic = "realtime:gdvn-pvp-" + std::to_string(m_matchID);

	auto socket = PvpRealtimeSocket::create(this);
	auto url = realtimeUrl(m_supabaseUrl, m_anonKey);

	if (!socket->connect(url)) {
		m_connecting = false;
		this->scheduleReconnect();
		return;
	}

	m_socket = socket;
}

void PvpOverlay::onRealtimeOpen() {
	if (!m_socket || m_cleanedUp) {
		return;
	}

	m_connecting = false;
	m_reconnectAttempts = 0;
	this->sendJoin();
}

void PvpOverlay::onRealtimeMessage(std::string const& message) {
	if (!m_socket || m_cleanedUp || message.empty()) {
		return;
	}

	try {
		auto json = matjson::parse(message).unwrap();
		this->handleRealtimeMessage(json);
	} catch (...) {
		log::warn("Failed to parse PvP realtime message");
	}
}

void PvpOverlay::onRealtimeClose() {
	m_socket.reset();
	m_connecting = false;
	m_joined = false;

	if (!m_cleanedUp && m_active) {
		this->scheduleReconnect();
	}
}

void PvpOverlay::handleRealtimeMessage(matjson::Value const& json) {
	auto event = getString(json, "event");

	if (event == "phx_reply") {
		if (getString(json["payload"], "status") == "ok") {
			m_joined = true;
		}
		return;
	}

	if (event == "postgres_changes") {
		auto const& payload = json["payload"];
		auto const& data = payload["data"];
		auto table = getString(data, "table");
		auto const& row = realtimeRecord(payload);

		if (table == "pvpMatchResults") {
			this->handleResultRow(row);
			this->refreshLabel();
		} else if (table == "pvpMatches") {
			this->handleMatchRow(row);
		}
	}
}

void PvpOverlay::handleResultRow(matjson::Value const& row) {
	auto uid = getString(row, "uid");
	auto progress = getNumber(row, "progress");

	if (uid.empty()) {
		return;
	}

	if (!m_currentUid.empty() && uid == m_currentUid) {
		m_self.uid = uid;
		m_self.progress = std::max(m_self.progress, progress);
		return;
	}

	m_opponent.uid = uid;
	m_opponent.progress = std::max(m_opponent.progress, progress);
}

void PvpOverlay::handleMatchRow(matjson::Value const& row) {
	m_active = this->isActiveStatus(getString(row, "status"));
	this->setOverlayVisible(m_active);

	if (!m_active) {
		this->closeSocket();
	}
}

void PvpOverlay::sendJoin() {
	auto changes = matjson::Value::array();
	auto matchID = std::to_string(m_matchID);
	changes.push(makeChange("INSERT", "pvpMatchResults", "matchId=eq." + matchID));
	changes.push(makeChange("UPDATE", "pvpMatchResults", "matchId=eq." + matchID));
	changes.push(makeChange("UPDATE", "pvpMatches", "id=eq." + matchID));

	auto broadcast = matjson::Value::object();
	broadcast["ack"] = false;
	broadcast["self"] = false;

	auto presence = matjson::Value::object();
	presence["enabled"] = false;

	auto config = matjson::Value::object();
	config["broadcast"] = broadcast;
	config["presence"] = presence;
	config["postgres_changes"] = changes;
	config["private"] = false;

	auto payload = matjson::Value::object();
	payload["config"] = config;

	auto ref = this->nextRef();
	auto message = matjson::Value::object();
	message["topic"] = m_topic;
	message["event"] = "phx_join";
	message["payload"] = payload;
	message["ref"] = ref;
	message["join_ref"] = ref;

	this->sendJson(message);
}

void PvpOverlay::sendHeartbeat() {
	auto message = matjson::Value::object();
	message["topic"] = "phoenix";
	message["event"] = "heartbeat";
	message["payload"] = matjson::Value::object();
	message["ref"] = this->nextRef();
	this->sendJson(message);
}

void PvpOverlay::sendJson(matjson::Value const& json) {
	if (!m_socket || !m_socket->isOpen()) {
		return;
	}

	m_socket->send(json.dump(matjson::NO_INDENTATION));
}

std::string PvpOverlay::nextRef() {
	return std::to_string(m_ref++);
}

void PvpOverlay::update(float dt) {
	if (m_cleanedUp) {
		return;
	}

	this->updateLabelPosition();

	if (m_reconnectTimer >= 0.0f) {
		m_reconnectTimer -= dt;
		if (m_reconnectTimer <= 0.0f) {
			m_reconnectTimer = -1.0f;
			this->requestRealtimeToken();
		}
	}

	if (m_socket && m_socket->isOpen()) {
		m_heartbeatTimer += dt;
		if (m_heartbeatTimer >= HEARTBEAT_INTERVAL) {
			m_heartbeatTimer = 0.0f;
			this->sendHeartbeat();
		}
	}
}

void PvpOverlay::scheduleReconnect() {
	if (m_cleanedUp || !m_active || m_connecting) {
		return;
	}

	m_reconnectAttempts = std::min(m_reconnectAttempts + 1, 4);
	m_reconnectTimer = static_cast<float>(std::min(1 << (m_reconnectAttempts - 1), 10));
}

void PvpOverlay::closeSocket() {
	if (!m_socket) {
		return;
	}

	auto socket = m_socket;
	m_socket.reset();
	socket->close();
}

void PvpOverlay::cleanup() {
	if (m_cleanedUp) {
		return;
	}

	m_cleanedUp = true;
	m_matchHolder.cancel();
	m_tokenHolder.cancel();
	this->closeSocket();

	if (m_label) {
		m_label->removeFromParentAndCleanup(true);
		m_label = nullptr;
	}
}

void PvpOverlay::refreshLabel() {
	if (!m_label) {
		return;
	}

	m_label->setString(fmt::format("PvP\nYou: {:.2f}%\nOpponent: {:.2f}%", m_self.progress, m_opponent.progress).c_str());
	this->setOverlayVisible(m_active);
	this->updateLabelPosition();
}

void PvpOverlay::updateLabelPosition() {
	if (!m_label) {
		return;
	}

	auto size = CCDirector::sharedDirector()->getWinSize();
	m_label->setAnchorPoint({ 0.0f, 1.0f });
	m_label->setPosition({ LABEL_MARGIN, size.height - LABEL_MARGIN });
}

void PvpOverlay::setOverlayVisible(bool visible) {
	if (m_label) {
		m_label->setVisible(visible && Mod::get()->getSettingValue<bool>("show-pvp-overlay"));
	}
}

bool PvpOverlay::isActiveStatus(std::string const& status) const {
	return status == "in_progress" || status == "waiting_result";
}
