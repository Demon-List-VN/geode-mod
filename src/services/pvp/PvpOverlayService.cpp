#include "PvpOverlayService.hpp"

#include "../../adapters/PvpMatchAdapter.hpp"
#include "../../adapters/PvpMessageAdapter.hpp"
#include "../../clients/auth/AuthClient.hpp"
#include "../../clients/level/LevelClient.hpp"
#include "../../clients/pvp/PvpClient.hpp"
#include "../../consts/ConfigConst.hpp"
#include "../../consts/PvpWebsocketEventConst.hpp"
#include "../../ui/components/pvp/PvpChatPopup.hpp"
#include "../../ui/components/pvp/PvpOverlay.hpp"
#include "../../ui/components/pvp/PvpRecentChatStack.hpp"
#include "../../utils/DateUtils.hpp"
#include "../../utils/StringUtils.hpp"
#include "../auth/AuthService.hpp"
#include "PvpSubmitterService.hpp"

#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace geode::prelude;

namespace gdvn::pvp_overlay_detail {
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
} // namespace gdvn::pvp_overlay_detail
using namespace gdvn::pvp_overlay_detail;

PvpOverlayService* PvpOverlayService::s_activeOverlay = nullptr;

PvpOverlayService::PvpOverlayService(PlayLayer* layer, int levelID, PvpSubmitterService* submitter)
    : m_layer(layer), m_submitter(submitter), m_levelID(levelID) {
    s_activeOverlay = this;
    m_chatMuted = Mod::get()->getSavedValue<bool>("pvp-chat-muted", false);
    m_overlay = std::make_unique<PvpOverlay>(m_layer);
    m_recentChatStack = std::make_unique<PvpRecentChatStack>(m_layer);
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

    m_chatPopup = PvpChatPopup::create(this);
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
        if (m_recentChatStack) {
            m_recentChatStack->clear();
        }
    }

    this->refreshChatVisibility();
}

void PvpOverlayService::notifyChatPopupClosed(PvpChatPopup* popup) {
    if (m_chatPopup == popup) {
        m_chatPopup = nullptr;
    }
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
    m_matchEndsAtEpoch = gdvn::utils::date::parseIsoEpochSeconds(snapshot.endsAt);
    m_lastCountdownSeconds = -1;
    auto status = snapshot.status;
    m_active = this->isActiveStatus(status);
    m_chatOpen = m_active || this->isCompletedStatus(status);
    m_chatGraceTimer = this->isCompletedStatus(status) ? CHAT_GRACE_SECONDS : -1.0f;

    m_self = {};
    m_opponent = {};

    for (auto const& participant : snapshot.participants) {
        if (participant.valid) {
            PvpOverlayPlayerProgressModel player;
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

    content = gdvn::utils::string::trimCopy(std::move(content));
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
    if (!this->isReadyForRealtime() || m_supabaseUrl.empty() || m_anonKey.empty() || m_websocketClient) {
        return;
    }

    m_connecting = true;

    auto client = PvpWebsocketClient::create({
        [this] { this->handleRealtimeOpen(); },
        [this](PvpMatchRealtimeMessageDto const& message) { this->handleRealtimeMessage(message); },
        [this] { this->handleRealtimeClose(); },
    });
    if (!client->connect(m_supabaseUrl, m_anonKey, m_matchID, m_realtimeAccessToken)) {
        m_connecting = false;
        this->scheduleReconnect();
        return;
    }

    m_websocketClient = client;
}

void PvpOverlayService::handleRealtimeOpen() {
    if (!m_websocketClient || m_cleanedUp) {
        return;
    }

    m_connecting = false;
    m_reconnectAttempts = 0;
}

void PvpOverlayService::handleRealtimeMessage(PvpMatchRealtimeMessageDto const& message) {
    if (!m_websocketClient || m_cleanedUp || !message.valid) {
        return;
    }

    if (message.table == gdvn::consts::PvpWebsocketEvent::RESULT_TABLE) {
        this->handleResultRow(PvpMatchAdapter::playerProgressFromJson(message.row));
        this->refreshLabel();
        this->scheduleMessageRefresh();
    } else if (message.table == gdvn::consts::PvpWebsocketEvent::MATCH_TABLE) {
        this->handleMatchRow(PvpMatchAdapter::matchRowFromJson(message.row));
        this->scheduleMessageRefresh();
    } else if (message.table == gdvn::consts::PvpWebsocketEvent::MESSAGE_TABLE) {
        log::info("Versus realtime chat event received: match={}, id={}", message.rowMatchID, message.rowID);
        this->handleMessageRow(PvpMessageAdapter::fromJson(message.row), true);
        this->scheduleMessageRefresh();
    }
}

void PvpOverlayService::handleRealtimeClose() {
    m_websocketClient.reset();
    m_connecting = false;

    if (!m_cleanedUp && m_chatOpen) {
        this->scheduleReconnect();
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
        m_matchEndsAtEpoch = gdvn::utils::date::parseIsoEpochSeconds(row.endsAt);
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
        this->closeRealtime();
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

    PvpOverlayChatMessageModel message;
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
        auto existing =
            std::find_if(m_chatMessages.begin(), m_chatMessages.end(),
                         [message](PvpOverlayChatMessageModel const& item) { return item.id == message.id; });

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

std::string PvpOverlayService::formatPlayerLabel(std::string const& label,
                                                 PvpOverlayPlayerProgressModel const& player) const {
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
        lines.push_back(
            gdvn::utils::string::truncate(gdvn::utils::string::toTTFSafeText(sender + ": " + message.content), 120));
    }

    return lines;
}

std::string PvpOverlayService::getChatSenderLabel(PvpOverlayChatMessageModel const& message) const {
    if (message.type == "system") {
        return "System";
    }

    if (!m_currentUid.empty() && message.senderUid == m_currentUid) {
        return "You";
    }

    return "Opponent";
}

void PvpOverlayService::pushRecentMessage(PvpOverlayChatMessageModel const& message) {
    if (!m_recentChatStack || m_chatMuted) {
        return;
    }

    auto sender = this->getChatSenderLabel(message);
    auto text = gdvn::utils::string::truncate(gdvn::utils::string::toTTFSafeText(sender + ": " + message.content), 140);
    if (text.empty()) {
        return;
    }

    m_recentChatStack->pushMessage(message.id, text, message.type,
                                   !m_currentUid.empty() && message.senderUid == m_currentUid);
    this->refreshChatVisibility();
}

void PvpOverlayService::updateRecentMessages(float dt) {
    if (m_recentChatStack) {
        m_recentChatStack->update(dt);
    }

    this->refreshChatVisibility();
}

void PvpOverlayService::update(float dt) {
    if (m_cleanedUp) {
        return;
    }

    if (m_overlay) {
        m_overlay->updatePosition();
    }
    if (m_recentChatStack) {
        m_recentChatStack->updatePosition();
    }
    this->updateRecentMessages(dt);

    if (m_active && m_matchEndsAtEpoch > 0) {
        auto countdownSeconds =
            std::max<std::int64_t>(0, m_matchEndsAtEpoch - gdvn::utils::date::currentEpochSeconds());
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
                this->closeRealtime();
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

    if (m_websocketClient && m_websocketClient->isOpen() && m_realtimeTokenExpiresAt > 0 &&
        gdvn::utils::date::currentEpochSeconds() >= m_realtimeTokenExpiresAt - 60) {
        m_realtimeTokenExpiresAt = 0;
        this->closeRealtime();
        this->requestRealtimeToken();
        return;
    }

    if (m_websocketClient) {
        m_websocketClient->update(dt);
    }
}

void PvpOverlayService::scheduleReconnect() {
    if (m_cleanedUp || !m_chatOpen || m_connecting) {
        return;
    }

    m_reconnectAttempts = std::min(m_reconnectAttempts + 1, 4);
    m_reconnectTimer = static_cast<float>(std::min(1 << (m_reconnectAttempts - 1), 10));
}

void PvpOverlayService::closeRealtime() {
    if (!m_websocketClient) {
        return;
    }

    auto client = m_websocketClient;
    m_websocketClient.reset();
    m_connecting = false;
    client->close();
}

void PvpOverlayService::cleanup() {
    if (m_cleanedUp) {
        return;
    }

    m_cleanedUp = true;
    this->closeRealtime();

    if (s_activeOverlay == this) {
        s_activeOverlay = nullptr;
    }

    if (m_chatPopup) {
        m_chatPopup->closeFromOverlay();
        m_chatPopup = nullptr;
    }

    m_recentChatStack.reset();
    m_overlay.reset();
}

void PvpOverlayService::refreshLabel() {
    if (!m_overlay) {
        return;
    }

    auto countdownSeconds =
        m_matchEndsAtEpoch > 0
            ? std::max<std::int64_t>(0, m_matchEndsAtEpoch - gdvn::utils::date::currentEpochSeconds())
            : -1;
    auto timerLine =
        countdownSeconds >= 0 ? fmt::format("\nTime: {}", gdvn::utils::date::formatCountdown(countdownSeconds)) : "";
    m_lastCountdownSeconds = countdownSeconds;

    m_overlay->setText(fmt::format("Versus{}\n{}\n{}", timerLine, formatPlayerLabel("You", m_self),
                                   formatPlayerLabel("Opponent", m_opponent)));
    this->setOverlayVisible(m_active);
}

void PvpOverlayService::refreshChatVisibility() {
    auto visible = m_chatOpen && !m_cleanedUp;

    if (!visible && m_chatPopup) {
        m_chatPopup->closeFromOverlay();
        m_chatPopup = nullptr;
    }

    if (m_recentChatStack) {
        m_recentChatStack->setVisible(visible && !m_chatMuted && m_recentChatStack->hasMessages());
    }
}

void PvpOverlayService::setOverlayVisible(bool visible) {
    if (m_overlay) {
        m_overlay->setVisible(visible);
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
