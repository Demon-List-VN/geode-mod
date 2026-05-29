#include "PvpOverlayService.hpp"

#include "../../adapters/PvpMatchAdapter.hpp"
#include "../../adapters/PvpMessageAdapter.hpp"
#include "../../clients/auth/AuthClient.hpp"
#include "../../clients/level/LevelClient.hpp"
#include "../../clients/pvp/PvpClient.hpp"
#include "../../common.hpp"
#include "../auth/AuthService.hpp"
#include "PvpSubmitterService.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>

using namespace geode::prelude;

namespace gdvn::pvp_overlay_detail {
constexpr float HEARTBEAT_INTERVAL = 25.0f;
constexpr float LABEL_MARGIN = 8.0f;
constexpr float CHAT_MARGIN = 8.0f;
constexpr float CHAT_MESSAGE_LIFETIME = 8.0f;
constexpr float CHAT_FADE_TIME = 1.5f;
constexpr float CHAT_GRACE_SECONDS = 3.0f * 60.0f;
constexpr float MESSAGE_REFRESH_COALESCE = 0.2f;
constexpr float CHAT_HISTORY_WIDTH = 320.0f;
constexpr float CHAT_HISTORY_LINE_HEIGHT = 15.0f;
constexpr int MAX_RECENT_MESSAGES = 4;
constexpr int CHAT_HISTORY_VISIBLE_LINES = 8;
constexpr int MAX_CHAT_MESSAGE_LENGTH = 500;
constexpr int MESSAGE_FETCH_LIMIT = 100;

PvpOverlayService* s_activeOverlay = nullptr;

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

std::string trimCopy(std::string value) {
    auto begin = std::find_if_not(value.begin(), value.end(), [=](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [=](unsigned char c) { return std::isspace(c); }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

std::string toLabelSafeText(std::string text) {
    std::string safe;
    safe.reserve(text.size());

    for (unsigned char c : text) {
        if (c == '\n' || c == '\r' || c == '\t') {
            safe.push_back(' ');
        } else if (c >= 32 && c < 127) {
            safe.push_back(static_cast<char>(c));
        }
    }

    return safe;
}

std::string toTTFSafeText(std::string text) {
    std::string safe;
    safe.reserve(text.size());

    for (unsigned char c : text) {
        if (c == '\n' || c == '\r' || c == '\t') {
            safe.push_back(' ');
        } else if (c >= 32 || c >= 128) {
            safe.push_back(static_cast<char>(c));
        }
    }

    return safe;
}

std::string truncateLabel(std::string text, size_t maxLength) {
    if (text.size() <= maxLength) {
        return text;
    }

    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }

    return text.substr(0, maxLength - 3) + "...";
}

std::string formatProgress(float value) {
    auto rounded = std::round(value);
    if (std::fabs(value - rounded) < 0.005f) {
        return fmt::format("{}", static_cast<int>(rounded));
    }

    auto text = fmt::format("{:.2f}", value);
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

std::string formatProgressForMode(float value, std::string const& mode) {
    if (mode == "platformer") {
        return fmt::format("{} PT", std::max(0, static_cast<int>(std::floor(value))));
    }

    return formatProgress(value) + "%";
}

std::string systemParticipantLabel(std::string const& uid, std::string const& currentUid) {
    if (!uid.empty() && !currentUid.empty() && uid == currentUid) {
        return "You";
    }

    return "Opponent";
}

std::int64_t currentEpochSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::int64_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const auto era = (year >= 0 ? year : year - 399) / 400;
    const auto yoe = static_cast<unsigned>(year - era * 400);
    const auto doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const auto doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

bool parseFixedInt(std::string const& value, size_t offset, size_t length, int& output) {
    if (offset + length > value.size()) {
        return false;
    }

    auto begin = value.data() + offset;
    auto end = begin + length;
    auto result = std::from_chars(begin, end, output);
    return result.ec == std::errc() && result.ptr == end;
}

std::int64_t parseIsoEpochSeconds(std::string const& value) {
    if (value.size() < 19) {
        return 0;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (value[4] != '-' || value[7] != '-' || value[10] != 'T' || value[13] != ':' || value[16] != ':' ||
        !parseFixedInt(value, 0, 4, year) || !parseFixedInt(value, 5, 2, month) || !parseFixedInt(value, 8, 2, day) ||
        !parseFixedInt(value, 11, 2, hour) || !parseFixedInt(value, 14, 2, minute) ||
        !parseFixedInt(value, 17, 2, second) || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 ||
        hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
        return 0;
    }

    return daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) * 86400 + hour * 3600 +
           minute * 60 + second;
}

std::string formatCountdown(std::int64_t seconds) {
    seconds = std::max<std::int64_t>(0, seconds);
    const auto minutes = seconds / 60;
    const auto secs = seconds % 60;
    return fmt::format("{}:{:02}", minutes, secs);
}

matjson::Value makeChange(std::string const& event, std::string const& table, std::string const& filter) {
    auto change = matjson::Value::object();
    change["event"] = event;
    change["schema"] = "public";
    change["table"] = table;
    change["filter"] = filter;
    return change;
}

} // namespace gdvn::pvp_overlay_detail
using namespace gdvn::pvp_overlay_detail;

class PvpChatPopupService final : public Popup, public TextInputDelegate {
  public:
    static PvpChatPopupService* create(PvpOverlayService* overlay) {
        auto ret = new PvpChatPopupService();
        if (ret->init(overlay)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    void focusInput() {
        if (m_input) {
            m_input->focus();
        }
    }

    void setSending(bool sending) {
        m_sending = sending;
        if (m_sendButton) {
            m_sendButton->setEnabled(!sending);
        }
    }

    void setStatus(std::string const& status) {
        if (m_statusLabel) {
            m_statusLabel->setString(status.c_str());
        }
    }

    void updateHistory() {
        if (!m_overlay) {
            return;
        }

        auto lines = m_overlay->getChatHistoryLines();
        if (lines.empty()) {
            lines.push_back("No messages yet.");
        }

        auto maxOffset = lines.size() > CHAT_HISTORY_VISIBLE_LINES ? lines.size() - CHAT_HISTORY_VISIBLE_LINES : 0;
        m_historyOffset = std::min(m_historyOffset, maxOffset);
        auto start = maxOffset - m_historyOffset;
        auto end = std::min(lines.size(), start + CHAT_HISTORY_VISIBLE_LINES);

        for (auto* label : m_historyLabels) {
            if (label) {
                label->removeFromParentAndCleanup(true);
            }
        }
        m_historyLabels.clear();

        for (size_t i = start; i < end; ++i) {
            auto label = CCLabelBMFont::create(lines[i].c_str(), "chatFont.fnt");
            label->setAnchorPoint({0.0f, 1.0f});
            label->setScale(0.52f);
            label->setOpacity(220);
            label->setColor(ccc3(235, 235, 235));
            label->limitLabelWidth(CHAT_HISTORY_WIDTH, 0.52f, 0.28f);
            m_mainLayer->addChildAtPosition(label, Anchor::TopLeft,
                                            {24.0f, -38.0f - CHAT_HISTORY_LINE_HEIGHT * static_cast<float>(i - start)});
            m_historyLabels.push_back(label);
        }

        if (m_upButton) {
            m_upButton->setEnabled(m_historyOffset < maxOffset);
        }
        if (m_downButton) {
            m_downButton->setEnabled(m_historyOffset > 0);
        }
    }

    void didSend() {
        m_sending = false;
        this->setSending(false);
        this->setStatus("");

        if (m_input) {
            m_input->setString("", false);
            m_input->focus();
        }

        this->updateHistory();
    }

    void closeFromOverlay() {
        m_overlay = nullptr;
        this->onClose(nullptr);
    }

    void scrollHistory(int delta) {
        auto lineCount = m_overlay ? m_overlay->getChatHistoryLines().size() : 0;
        auto maxOffset = lineCount > CHAT_HISTORY_VISIBLE_LINES ? lineCount - CHAT_HISTORY_VISIBLE_LINES : 0;
        auto next = static_cast<int>(m_historyOffset) + delta;
        m_historyOffset = static_cast<size_t>(std::clamp(next, 0, static_cast<int>(maxOffset)));
        this->updateHistory();
    }

  protected:
    bool init(PvpOverlayService* overlay) {
        if (!Popup::init(380.0f, 220.0f)) {
            return false;
        }

        m_overlay = overlay;
        this->setTitle("Versus Chat");

        auto upSprite = ButtonSprite::create("Up", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        upSprite->setScale(0.4f);
        m_upButton = CCMenuItemExt::createSpriteExtra(upSprite, [this](auto*) { this->scrollHistory(1); });
        m_buttonMenu->addChildAtPosition(m_upButton, Anchor::TopRight, {-28.0f, -48.0f});

        auto downSprite = ButtonSprite::create("Down", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        downSprite->setScale(0.4f);
        m_downButton = CCMenuItemExt::createSpriteExtra(downSprite, [this](auto*) { this->scrollHistory(-1); });
        m_buttonMenu->addChildAtPosition(m_downButton, Anchor::TopRight, {-28.0f, -86.0f});

        m_input = TextInput::create(250.0f, "Type a message...", "chatFont.fnt");
        m_input->setCommonFilter(CommonFilter::Any);
        m_input->setMaxCharCount(MAX_CHAT_MESSAGE_LENGTH);
        m_input->setTextAlign(TextInputAlign::Left);
        m_input->setDelegate(this);
        m_mainLayer->addChildAtPosition(m_input, Anchor::Bottom, {-38.0f, 38.0f});

        auto sendSprite = ButtonSprite::create("Send", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        sendSprite->setScale(0.55f);
        m_sendButton = CCMenuItemExt::createSpriteExtra(sendSprite, [this](auto*) { this->submit(); });
        m_buttonMenu->addChildAtPosition(m_sendButton, Anchor::Bottom, {128.0f, 38.0f});

        m_statusLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_statusLabel->setScale(0.32f);
        m_statusLabel->setOpacity(180);
        m_mainLayer->addChildAtPosition(m_statusLabel, Anchor::Bottom, {0.0f, 16.0f});

        this->updateHistory();

        return true;
    }

    void onClose(CCObject* sender) override {
        for (auto*& label : m_historyLabels) {
            if (label) {
                label->removeFromParentAndCleanup(true);
                label = nullptr;
            }
        }
        m_historyLabels.clear();

        if (m_overlay) {
            auto overlay = m_overlay;
            m_overlay = nullptr;
            overlay->notifyChatPopupClosed(this);
        }

        Popup::onClose(sender);
    }

    void textChanged(CCTextInputNode*) override {
    }

    void textInputReturn(CCTextInputNode*) override {
        this->submit();
    }

    void enterPressed(CCTextInputNode*) override {
        this->submit();
    }

  private:
    PvpOverlayService* m_overlay = nullptr;
    TextInput* m_input = nullptr;
    std::vector<CCLabelBMFont*> m_historyLabels;
    CCLabelBMFont* m_statusLabel = nullptr;
    CCMenuItemSpriteExtra* m_sendButton = nullptr;
    CCMenuItemSpriteExtra* m_upButton = nullptr;
    CCMenuItemSpriteExtra* m_downButton = nullptr;
    size_t m_historyOffset = 0;
    bool m_sending = false;

    void submit() {
        if (!m_overlay || !m_input || m_sending) {
            return;
        }

        auto content = trimCopy(static_cast<std::string>(m_input->getString()));
        if (content.empty()) {
            this->setStatus("Message cannot be empty");
            return;
        }

        this->setSending(true);
        this->setStatus("Sending...");
        m_overlay->submitChatMessage(std::move(content));
    }
};

PvpOverlayService::PvpOverlayService(PlayLayer* layer, int levelID, PvpSubmitterService* submitter)
    : m_layer(layer), m_submitter(submitter), m_levelID(levelID) {
    s_activeOverlay = this;
    m_chatMuted = Mod::get()->getSavedValue<bool>("pvp-chat-muted", false);
    this->createLabel();
    this->createChatNodes();
    this->requestMatch();
}

PvpOverlayService::~PvpOverlayService() {
    this->cleanup();
}

PvpOverlayService* PvpOverlayService::getActive() {
    return s_activeOverlay;
}

bool PvpOverlayService::isChatMuted() const {
    return m_chatMuted;
}

bool PvpOverlayService::hasPvpMatch() const {
    return m_matchID > 0 && m_chatOpen && !m_cleanedUp;
}

bool PvpOverlayService::isChatUsable() const {
    return m_matchID > 0 && m_chatOpen && !m_cleanedUp;
}

bool PvpOverlayService::openChat() {
    if (!this->isChatUsable()) {
        return false;
    }

    if (m_chatPopup) {
        this->requestMessages(false, false);
        m_chatPopup->updateHistory();
        m_chatPopup->focusInput();
        return true;
    }

    m_chatPopup = PvpChatPopupService::create(this);
    if (!m_chatPopup) {
        return false;
    }

    m_chatPopup->show();
    this->requestMessages(false, false);
    m_chatPopup->updateHistory();
    m_chatPopup->focusInput();
    return true;
}

void PvpOverlayService::setChatMuted(bool muted) {
    if (m_chatMuted == muted) {
        return;
    }

    m_chatMuted = muted;
    Mod::get()->setSavedValue<bool>("pvp-chat-muted", muted);

    if (muted) {
        for (auto& message : m_recentMessages) {
            if (message.label) {
                message.label->removeFromParentAndCleanup(true);
            }
        }
        m_recentMessages.clear();
    }

    this->refreshChatVisibility();
}

void PvpOverlayService::notifyChatPopupClosed(PvpChatPopupService* popup) {
    if (m_chatPopup == popup) {
        m_chatPopup = nullptr;
    }
}

void PvpOverlayService::createLabel() {
    if (!m_layer) {
        return;
    }

    m_label = CCLabelBMFont::create("Versus\nYou: 0.00%\nOpponent: 0.00%", "bigFont.fnt");
    m_label->setScale(0.32f);
    m_label->setOpacity(180);
    m_label->setVisible(false);
    m_label->setID("gdvn-pvp-overlay"_spr);

    auto parent = m_layer->m_uiLayer ? static_cast<CCNode*>(m_layer->m_uiLayer) : static_cast<CCNode*>(m_layer);
    parent->addChild(m_label, 1000);
    this->updateLabelPosition();
}

void PvpOverlayService::createChatNodes() {
    if (!m_layer) {
        return;
    }

    auto parent = m_layer->m_uiLayer ? static_cast<CCNode*>(m_layer->m_uiLayer) : static_cast<CCNode*>(m_layer);

    m_chatStack = CCNode::create();
    m_chatStack->setID("gdvn-pvp-chat-stack"_spr);
    m_chatStack->setVisible(false);
    parent->addChild(m_chatStack, 1001);

    this->updateChatPositions();
    this->refreshChatVisibility();
}

void PvpOverlayService::requestMatch() {
    if (!AuthService::isLoggedIn() || m_cleanedUp) {
        return;
    }

    LevelClient::getActivePvpMatch(m_levelID, [this](ActivePvpMatchResponseDto const& match, web::WebResponse& res) {
        if (m_cleanedUp) {
            return;
        }

        if (!res.ok()) {
            this->setOverlayVisible(false);
            return;
        }

        if (!match.valid) {
            log::warn("Failed to map Versus overlay match snapshot");
            return;
        }

        this->parseMatchSnapshot(PvpMatchAdapter::matchSnapshotFromJson(match.rawJson));
        this->refreshLabel();

        if (this->isReadyForRealtime()) {
            this->requestRealtimeToken();
        }
    });
}

void PvpOverlayService::parseMatchSnapshot(PvpMatchSnapshotDto const& snapshot) {
    m_matchID = snapshot.matchID;
    m_currentUid = snapshot.currentUid;
    m_mode = snapshot.mode == "platformer" ? "platformer" : "classic";
    m_matchEndsAtEpoch = parseIsoEpochSeconds(snapshot.endsAt);
    m_lastCountdownSeconds = -1;
    auto status = snapshot.status;
    m_active = this->isActiveStatus(status);
    m_chatOpen = m_active || this->isCompletedStatus(status);
    m_chatGraceTimer = this->isCompletedStatus(status) ? CHAT_GRACE_SECONDS : -1.0f;

    m_self = {};
    m_opponent = {};

    for (auto const& participant : snapshot.participants) {
        if (participant.valid) {
            PlayerProgress player;
            player.uid = participant.uid;
            player.progress = participant.progress;

            if (!m_currentUid.empty() && player.uid == m_currentUid) {
                m_self = player;
            } else if (m_opponent.uid.empty()) {
                m_opponent = player;
            }
        }
    }

    for (auto const& result : snapshot.results) {
        this->handleResultRow(result);
    }

    this->setOverlayVisible(m_active);
    this->refreshChatVisibility();

    if (m_matchID > 0) {
        this->requestMessages(false, false);
    }
}

void PvpOverlayService::requestRealtimeToken() {
    if (m_cleanedUp || m_requestingRealtimeToken) {
        return;
    }

    m_requestingRealtimeToken = true;

    AuthClient::getRealtimeToken([this](RealtimeTokenResponseDto const& token, web::WebResponse& res) {
        m_requestingRealtimeToken = false;

        if (m_cleanedUp) {
            return;
        }

        if (!res.ok()) {
            log::warn("Failed to get Versus realtime token: HTTP {}", res.code());
            return;
        }

        if (!token.valid) {
            log::warn("Failed to map Versus realtime token");
            return;
        }

        m_supabaseUrl = token.supabaseUrl;
        m_anonKey = token.anonKey;
        m_realtimeAccessToken = token.accessToken;
        m_realtimeTokenExpiresAt = token.expiresAt;
        this->connectRealtime();
    });
}

void PvpOverlayService::requestMessages(bool animateNew, bool incremental) {
    if (!AuthService::isLoggedIn() || m_cleanedUp || m_matchID <= 0) {
        return;
    }

    auto afterID = incremental ? m_latestMessageID : 0;
    auto limit = incremental ? MESSAGE_FETCH_LIMIT : 0;

    PvpClient::getMessages(m_matchID, afterID, limit,
                           [this, animateNew](PvpMessagesResponseDto const& messages, web::WebResponse& res) {
                               if (m_cleanedUp) {
                                   return;
                               }

                               if (!res.ok()) {
                                   log::warn("Failed to load Versus chat messages: HTTP {}", res.code());
                                   return;
                               }

                               if (!messages.valid) {
                                   log::warn("Failed to map Versus chat messages");
                                   return;
                               }

                               this->handleMessagesPayload(messages, animateNew);
                           });
}

void PvpOverlayService::submitChatMessage(std::string content) {
    if (!this->isChatUsable() || m_chatSending || m_matchID <= 0) {
        if (m_chatPopup) {
            m_chatPopup->setSending(false);
            m_chatPopup->setStatus("Chat is unavailable");
        }
        return;
    }

    content = trimCopy(std::move(content));
    if (content.empty()) {
        if (m_chatPopup) {
            m_chatPopup->setSending(false);
            m_chatPopup->setStatus("Message cannot be empty");
        }
        return;
    }

    if (content.size() > MAX_CHAT_MESSAGE_LENGTH) {
        content = content.substr(0, MAX_CHAT_MESSAGE_LENGTH);
    }

    m_chatSending = true;

    PvpClient::postMessage(m_matchID, content, [this](PvpMessageDto const& message, web::WebResponse& res) {
        m_chatSending = false;

        if (m_cleanedUp) {
            return;
        }

        if (!res.ok()) {
            log::warn("Failed to send Versus chat message: HTTP {}", res.code());
            if (m_chatPopup) {
                m_chatPopup->setSending(false);
                m_chatPopup->setStatus("Failed to send message");
            }
            Notification::create("Failed to send Versus chat message", NotificationIcon::Error, 2.0f)->show();
            return;
        }

        if (!message.valid) {
            log::warn("Failed to map sent Versus chat message");
        } else {
            this->handleMessageRow(message, true);
        }

        if (m_chatPopup) {
            m_chatPopup->didSend();
        }
    });
}

bool PvpOverlayService::isReadyForRealtime() const {
    return m_matchID > 0 && m_chatOpen && !m_cleanedUp;
}

void PvpOverlayService::connectRealtime() {
    if (!this->isReadyForRealtime() || m_supabaseUrl.empty() || m_anonKey.empty() || m_socket) {
        return;
    }

    m_connecting = true;
    m_joined = false;
    m_heartbeatTimer = 0.0f;
    m_topic = "realtime:gdvn-pvp-" + std::to_string(m_matchID);

    auto socket = PvpRealtimeSocketService::create(this);
    auto url = realtimeUrl(m_supabaseUrl, m_anonKey);

    if (!socket->connect(url)) {
        m_connecting = false;
        this->scheduleReconnect();
        return;
    }

    m_socket = socket;
}

void PvpOverlayService::onRealtimeOpen() {
    if (!m_socket || m_cleanedUp) {
        return;
    }

    m_connecting = false;
    m_reconnectAttempts = 0;
    this->sendJoin();
}

void PvpOverlayService::onRealtimeMessage(std::string const& message) {
    if (!m_socket || m_cleanedUp || message.empty()) {
        return;
    }

    auto dto = PvpMatchAdapter::realtimeMessageFromString(message);
    if (!dto.valid) {
        log::warn("Failed to parse Versus realtime message");
        return;
    }

    this->handleRealtimeMessage(dto);
}

void PvpOverlayService::onRealtimeClose() {
    m_socket.reset();
    m_connecting = false;
    m_joined = false;

    if (!m_cleanedUp && m_chatOpen) {
        this->scheduleReconnect();
    }
}

void PvpOverlayService::handleRealtimeMessage(PvpMatchRealtimeMessageDto const& message) {
    if (message.event == "phx_reply") {
        if (message.replyOk) {
            m_joined = true;
        } else {
            log::warn("Failed to join Versus realtime channel");
            if (!m_realtimeAccessToken.empty()) {
                m_realtimeAccessToken.clear();
                this->sendJoin();
            }
        }
        return;
    }

    if (message.event == "postgres_changes") {
        if (message.table == "pvpMatchResults") {
            this->handleResultRow(PvpMatchAdapter::playerProgressFromJson(message.row));
            this->refreshLabel();
            this->scheduleMessageRefresh();
        } else if (message.table == "pvpMatches") {
            this->handleMatchRow(PvpMatchAdapter::matchRowFromJson(message.row));
            this->scheduleMessageRefresh();
        } else if (message.table == "pvpMatchMessages") {
            log::info("Versus realtime chat event received: match={}, id={}", message.rowMatchID, message.rowID);
            this->handleMessageRow(PvpMessageAdapter::fromJson(message.row), true);
            this->scheduleMessageRefresh();
        }
    }
}

void PvpOverlayService::handleResultRow(PvpMatchPlayerProgressDto const& row) {
    if (!row.valid) {
        return;
    }

    if (!m_currentUid.empty() && row.uid == m_currentUid) {
        m_self.uid = row.uid;
        m_self.progress = std::max(m_self.progress, row.progress);
        return;
    }

    m_opponent.uid = row.uid;
    m_opponent.progress = std::max(m_opponent.progress, row.progress);
    if (row.progress >= 100.0f && m_submitter) {
        m_submitter->flushDeathCount();
    }
}

void PvpOverlayService::handleMatchRow(PvpMatchRowDto const& row) {
    const bool wasActive = m_active;
    if (row.mode == "platformer") {
        m_mode = "platformer";
    }
    if (!row.endsAt.empty()) {
        m_matchEndsAtEpoch = parseIsoEpochSeconds(row.endsAt);
        m_lastCountdownSeconds = -1;
    }
    auto status = row.status;
    m_active = this->isActiveStatus(status);
    m_chatOpen = m_active || this->isCompletedStatus(status);
    m_chatGraceTimer = this->isCompletedStatus(status) ? CHAT_GRACE_SECONDS : -1.0f;
    if (wasActive && this->isCompletedStatus(status) && m_submitter) {
        m_submitter->flushDeathCount();
    }
    this->refreshLabel();
    this->setOverlayVisible(m_active);
    this->refreshChatVisibility();

    if (!m_chatOpen) {
        this->closeSocket();
    }
}

void PvpOverlayService::scheduleMessageRefresh() {
    if (m_cleanedUp || m_matchID <= 0) {
        return;
    }

    m_messageRefreshTimer = MESSAGE_REFRESH_COALESCE;
}

void PvpOverlayService::handleMessagesPayload(PvpMessagesResponseDto const& messages, bool animateNew) {
    for (auto const& message : messages.messages) {
        this->handleMessageRow(message, animateNew);
    }
}

void PvpOverlayService::handleMessageRow(PvpMessageDto const& dto, bool animateNew) {
    if (!dto.valid) {
        return;
    }

    ChatMessage message;
    message.id = dto.id;
    message.senderUid = dto.senderUid;
    message.type = dto.type;
    message.senderAnonymous = dto.senderAnonymous;
    auto isProgressSystemMessage = false;
    auto isHiddenSystemMessage = false;

    if (message.type == "system") {
        auto metadata = PvpMatchAdapter::systemMetadataFromJson(dto.metadata);
        auto kind = metadata.kind;
        isProgressSystemMessage = kind == "progress";
        isHiddenSystemMessage = kind == "play_mode";
        this->handleSystemMetadata(metadata);

        if (isHiddenSystemMessage) {
            if (message.id > m_latestMessageID) {
                m_latestMessageID = message.id;
            }
            this->refreshLabel();
            return;
        }

        message.content = this->formatSystemMessage(metadata);
    } else {
        message.content = dto.content;
    }

    if (message.content.empty()) {
        return;
    }

    if (message.id > 0) {
        auto existing = std::find_if(m_chatMessages.begin(), m_chatMessages.end(),
                                     [message](ChatMessage const& item) { return item.id == message.id; });

        if (existing == m_chatMessages.end()) {
            m_chatMessages.push_back(message);
        } else {
            *existing = message;
        }
    } else {
        m_chatMessages.push_back(message);
    }

    auto previousLatest = m_latestMessageID;
    if (message.id > m_latestMessageID) {
        m_latestMessageID = message.id;
    }

    log::info("Versus chat message received: match={}, id={}, type={}, sender={}, new={}, toast={}", m_matchID,
              message.id, message.type, message.senderUid, message.id > previousLatest,
              animateNew && message.id > previousLatest && !isProgressSystemMessage);

    if (animateNew && message.id > previousLatest && !isProgressSystemMessage) {
        this->pushRecentMessage(message);
    }

    if (m_chatPopup) {
        m_chatPopup->updateHistory();
    }
}

void PvpOverlayService::handleSystemMetadata(PvpMatchSystemMetadataDto const& metadata) {
    if (!metadata.valid) {
        return;
    }

    auto kind = metadata.kind;

    if (kind == "level_changed") {
        m_self.playMode = "normal";
        m_opponent.playMode = "normal";
        this->refreshLabel();
        return;
    }

    if (kind != "play_mode") {
        return;
    }

    auto uid = metadata.uid;
    auto playMode = metadata.playMode == "practice" ? "practice" : "normal";

    if (!m_currentUid.empty() && uid == m_currentUid) {
        m_self.uid = uid;
        m_self.playMode = playMode;
    } else if (!uid.empty()) {
        if (m_opponent.uid.empty()) {
            m_opponent.uid = uid;
        }
        if (m_opponent.uid == uid) {
            m_opponent.playMode = playMode;
        }
    }

    this->refreshLabel();
}

std::string PvpOverlayService::formatSystemMessage(PvpMatchSystemMetadataDto const& metadata) const {
    if (!metadata.valid) {
        return "Match update.";
    }

    auto kind = metadata.kind;
    if (kind == "progress") {
        auto progress = metadata.progress;
        auto mode = metadata.mode == "platformer" ? "platformer" : m_mode;
        auto player = systemParticipantLabel(metadata.uid, m_currentUid);
        auto formattedProgress = formatProgressForMode(progress, mode);
        if (mode == "platformer") {
            return fmt::format("{} reached {}.", player, formattedProgress);
        }

        return fmt::format("{} reached {} progress.", player, formattedProgress);
    }

    if (kind == "match_end") {
        auto winnerUid = metadata.winnerUid;
        if (winnerUid.empty()) {
            return "The match ended in a draw. Chat will remain open briefly.";
        }

        return fmt::format("{} won the match. Chat will remain open briefly.",
                           systemParticipantLabel(winnerUid, m_currentUid));
    }

    if (kind == "resignation") {
        auto resigning = systemParticipantLabel(metadata.resigningUid, m_currentUid);
        auto winnerUid = metadata.winnerUid;
        if (winnerUid.empty()) {
            return fmt::format("{} resigned. The match ended.", resigning);
        }

        return fmt::format("{} resigned. {} won the match. Chat will remain open briefly.", resigning,
                           systemParticipantLabel(winnerUid, m_currentUid));
    }

    if (kind == "level_change_requested") {
        return fmt::format("{} requested a level change. The level will change if both players agree.",
                           systemParticipantLabel(metadata.requesterUid, m_currentUid));
    }

    if (kind == "level_changed") {
        auto nextLevelID = metadata.nextLevelID;
        if (nextLevelID > 0) {
            return fmt::format("The match level changed to #{}. Progress and timer were reset.", nextLevelID);
        }

        return "The match level changed. Progress and timer were reset.";
    }

    return "Match update.";
}

std::string PvpOverlayService::formatPlayerLabel(std::string const& label, PlayerProgress const& player) const {
    auto modeSuffix = player.playMode == "practice" ? " (practice)" : "";
    return fmt::format("{}{}: {}", label, modeSuffix, formatProgressLabel(player.progress));
}

std::string PvpOverlayService::getChatHistoryText() const {
    auto lines = this->getChatHistoryLines();
    std::string text;

    for (auto const& line : lines) {
        if (!text.empty()) {
            text += "\n";
        }
        text += line;
    }

    return text;
}

std::vector<std::string> PvpOverlayService::getChatHistoryLines() const {
    std::vector<std::string> lines;
    lines.reserve(m_chatMessages.size());

    for (auto const& message : m_chatMessages) {
        auto sender = this->getChatSenderLabel(message);
        lines.push_back(truncateLabel(toTTFSafeText(sender + ": " + message.content), 120));
    }

    return lines;
}

std::string PvpOverlayService::getChatSenderLabel(ChatMessage const& message) const {
    if (message.type == "system") {
        return "System";
    }

    if (!m_currentUid.empty() && message.senderUid == m_currentUid) {
        return "You";
    }

    return "Opponent";
}

void PvpOverlayService::pushRecentMessage(ChatMessage const& message) {
    if (!m_chatStack || m_chatMuted) {
        return;
    }

    auto sender = this->getChatSenderLabel(message);
    auto text = truncateLabel(toTTFSafeText(sender + ": " + message.content), 140);
    if (text.empty()) {
        return;
    }

    auto label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
    label->setAnchorPoint({0.0f, 0.0f});
    label->setScale(0.55f);
    label->setOpacity(220);

    if (message.type == "system") {
        label->setColor(ccc3(255, 225, 120));
    } else if (!m_currentUid.empty() && message.senderUid == m_currentUid) {
        label->setColor(ccc3(150, 255, 150));
    }

    label->limitLabelWidth(280.0f, 0.55f, 0.3f);
    m_chatStack->addChild(label);

    m_recentMessages.push_back({message.id, CHAT_MESSAGE_LIFETIME, label});

    while (m_recentMessages.size() > MAX_RECENT_MESSAGES) {
        if (auto oldLabel = m_recentMessages.front().label) {
            oldLabel->removeFromParentAndCleanup(true);
        }
        m_recentMessages.erase(m_recentMessages.begin());
    }

    this->layoutRecentMessages();
    this->refreshChatVisibility();
}

void PvpOverlayService::layoutRecentMessages() {
    for (size_t i = 0; i < m_recentMessages.size(); ++i) {
        if (auto label = m_recentMessages[i].label) {
            auto reverseIndex = m_recentMessages.size() - i - 1;
            label->setPosition({0.0f, static_cast<float>(reverseIndex) * 12.0f});
        }
    }
}

void PvpOverlayService::updateRecentMessages(float dt) {
    if (m_recentMessages.empty()) {
        return;
    }

    for (auto& message : m_recentMessages) {
        message.timeLeft -= dt;
        if (message.label) {
            auto opacity = message.timeLeft < CHAT_FADE_TIME
                               ? static_cast<unsigned char>(std::max(0.0f, message.timeLeft / CHAT_FADE_TIME) * 220.0f)
                               : static_cast<unsigned char>(220);
            message.label->setOpacity(opacity);
        }
    }

    for (auto it = m_recentMessages.begin(); it != m_recentMessages.end();) {
        if (it->timeLeft <= 0.0f) {
            if (it->label) {
                it->label->removeFromParentAndCleanup(true);
            }
            it = m_recentMessages.erase(it);
        } else {
            ++it;
        }
    }

    this->layoutRecentMessages();
    this->refreshChatVisibility();
}

void PvpOverlayService::sendJoin() {
    auto changes = matjson::Value::array();
    auto matchID = std::to_string(m_matchID);
    changes.push(makeChange("INSERT", "pvpMatchResults", "matchId=eq." + matchID));
    changes.push(makeChange("UPDATE", "pvpMatchResults", "matchId=eq." + matchID));
    changes.push(makeChange("UPDATE", "pvpMatches", "id=eq." + matchID));
    changes.push(makeChange("INSERT", "pvpMatchMessages", "matchId=eq." + matchID));

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
    if (!m_realtimeAccessToken.empty()) {
        payload["access_token"] = m_realtimeAccessToken;
    }

    auto ref = this->nextRef();
    auto message = matjson::Value::object();
    message["topic"] = m_topic;
    message["event"] = "phx_join";
    message["payload"] = payload;
    message["ref"] = ref;
    message["join_ref"] = ref;

    this->sendJson(message);
}

void PvpOverlayService::sendHeartbeat() {
    auto message = matjson::Value::object();
    message["topic"] = "phoenix";
    message["event"] = "heartbeat";
    message["payload"] = matjson::Value::object();
    message["ref"] = this->nextRef();
    this->sendJson(message);
}

void PvpOverlayService::sendJson(matjson::Value const& json) {
    if (!m_socket || !m_socket->isOpen()) {
        return;
    }

    m_socket->send(json.dump(matjson::NO_INDENTATION));
}

std::string PvpOverlayService::nextRef() {
    return std::to_string(m_ref++);
}

void PvpOverlayService::update(float dt) {
    if (m_cleanedUp) {
        return;
    }

    this->updateLabelPosition();
    this->updateChatPositions();
    this->updateRecentMessages(dt);

    if (m_active && m_matchEndsAtEpoch > 0) {
        auto countdownSeconds = std::max<std::int64_t>(0, m_matchEndsAtEpoch - currentEpochSeconds());
        if (countdownSeconds != m_lastCountdownSeconds) {
            if (countdownSeconds == 0 && m_lastCountdownSeconds != 0 && m_submitter) {
                m_submitter->flushDeathCount();
            }
            this->refreshLabel();
        }
    }

    if (m_chatGraceTimer >= 0.0f) {
        m_chatGraceTimer -= dt;
        if (m_chatGraceTimer <= 0.0f) {
            m_chatGraceTimer = -1.0f;
            if (!m_active) {
                m_chatOpen = false;
                this->refreshChatVisibility();
                this->closeSocket();
            }
        }
    }

    if (m_messageRefreshTimer >= 0.0f) {
        m_messageRefreshTimer -= dt;
        if (m_messageRefreshTimer <= 0.0f) {
            m_messageRefreshTimer = -1.0f;
            this->requestMessages(true, true);
        }
    }

    if (m_reconnectTimer >= 0.0f) {
        m_reconnectTimer -= dt;
        if (m_reconnectTimer <= 0.0f) {
            m_reconnectTimer = -1.0f;
            this->requestRealtimeToken();
        }
    }

    if (m_socket && m_socket->isOpen() && m_realtimeTokenExpiresAt > 0 &&
        currentEpochSeconds() >= m_realtimeTokenExpiresAt - 60) {
        m_realtimeTokenExpiresAt = 0;
        this->closeSocket();
        this->requestRealtimeToken();
        return;
    }

    if (m_socket && m_socket->isOpen()) {
        m_heartbeatTimer += dt;
        if (m_heartbeatTimer >= HEARTBEAT_INTERVAL) {
            m_heartbeatTimer = 0.0f;
            this->sendHeartbeat();
        }
    }
}

void PvpOverlayService::scheduleReconnect() {
    if (m_cleanedUp || !m_chatOpen || m_connecting) {
        return;
    }

    m_reconnectAttempts = std::min(m_reconnectAttempts + 1, 4);
    m_reconnectTimer = static_cast<float>(std::min(1 << (m_reconnectAttempts - 1), 10));
}

void PvpOverlayService::closeSocket() {
    if (!m_socket) {
        return;
    }

    auto socket = m_socket;
    m_socket.reset();
    m_connecting = false;
    m_joined = false;
    socket->close();
}

void PvpOverlayService::cleanup() {
    if (m_cleanedUp) {
        return;
    }

    m_cleanedUp = true;
    this->closeSocket();

    if (s_activeOverlay == this) {
        s_activeOverlay = nullptr;
    }

    if (m_chatPopup) {
        m_chatPopup->closeFromOverlay();
        m_chatPopup = nullptr;
    }

    if (m_label) {
        m_label->removeFromParentAndCleanup(true);
        m_label = nullptr;
    }

    if (m_chatStack) {
        m_chatStack->removeFromParentAndCleanup(true);
        m_chatStack = nullptr;
    }
}

void PvpOverlayService::refreshLabel() {
    if (!m_label) {
        return;
    }

    auto countdownSeconds =
        m_matchEndsAtEpoch > 0 ? std::max<std::int64_t>(0, m_matchEndsAtEpoch - currentEpochSeconds()) : -1;
    auto timerLine = countdownSeconds >= 0 ? fmt::format("\nTime: {}", formatCountdown(countdownSeconds)) : "";
    m_lastCountdownSeconds = countdownSeconds;

    m_label->setString(fmt::format("Versus{}\n{}\n{}", timerLine, formatPlayerLabel("You", m_self),
                                   formatPlayerLabel("Opponent", m_opponent))
                           .c_str());
    this->setOverlayVisible(m_active);
    this->updateLabelPosition();
}

void PvpOverlayService::refreshChatVisibility() {
    auto visible = m_chatOpen && !m_cleanedUp;

    if (!visible && m_chatPopup) {
        m_chatPopup->closeFromOverlay();
        m_chatPopup = nullptr;
    }

    if (m_chatStack) {
        m_chatStack->setVisible(visible && !m_chatMuted && !m_recentMessages.empty());
    }
}

void PvpOverlayService::updateLabelPosition() {
    if (!m_label) {
        return;
    }

    auto size = CCDirector::sharedDirector()->getWinSize();
    m_label->setAnchorPoint({0.0f, 1.0f});
    m_label->setPosition({LABEL_MARGIN, size.height - LABEL_MARGIN});
}

void PvpOverlayService::updateChatPositions() {
    if (m_chatStack) {
        m_chatStack->setPosition({CHAT_MARGIN, CHAT_MARGIN});
    }
}

void PvpOverlayService::setOverlayVisible(bool visible) {
    if (m_label) {
        m_label->setVisible(visible && Mod::get()->getSettingValue<bool>("show-pvp-overlay"));
    }
}

bool PvpOverlayService::isActiveStatus(std::string const& status) const {
    return status == "in_progress" || status == "waiting_result";
}

bool PvpOverlayService::isCompletedStatus(std::string const& status) const {
    return status == "completed";
}

bool PvpOverlayService::isPlatformerMode() const {
    return m_mode == "platformer";
}

std::string PvpOverlayService::formatProgressLabel(float progress) const {
    return formatProgressForMode(progress, m_mode);
}
