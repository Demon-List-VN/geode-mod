#include "PvpOverlayService.hpp"

#include "../../adapters/PvpMatchAdapter.hpp"
#include "../../adapters/PvpMessageAdapter.hpp"
#include "../../clients/auth/AuthClient.hpp"
#include "../../clients/level/LevelClient.hpp"
#include "../../clients/pvp/PvpClient.hpp"
#include "../../consts/ConfigConst.hpp"
#include "../../consts/TableConst.hpp"
#include "../../consts/WebsocketEventConst.hpp"
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

std::string formatScore(float value, int targetScore) {
    auto current = formatProgress(value);
    if (targetScore > 0) {
        return fmt::format("{}/{}", current, targetScore);
    }

    return current;
}

std::string formatHp(float value, int startingHp) {
    auto current = formatProgress(value);
    if (startingHp > 0) {
        return fmt::format("{}/{} HP", current, startingHp);
    }

    return current + " HP";
}

std::string normalizeScoringMode(std::string const& mode) {
    return mode == "score" || mode == "hp" ? mode : "progress";
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

    log::info("Fetching Versus overlay match for level {}", m_levelID);

    LevelClient::getActivePvpMatch(m_levelID, [this](ActivePvpMatchResponseDto const& match, web::WebResponse& res) {
        if (m_cleanedUp) {
            return;
        }

        if (!res.ok()) {
            if (res.code() == 404) {
                log::info("No active Versus overlay match found for level {} (HTTP 404)", m_levelID);
            } else {
                log::warn("Failed to fetch Versus overlay match for level {}: HTTP {}", m_levelID, res.code());
            }
            this->setOverlayVisible(false);
            return;
        }

        if (!match.valid) {
            log::warn("Failed to map Versus overlay match snapshot");
            return;
        }

        auto snapshot = PvpMatchAdapter::matchSnapshotFromJson(match.rawJson);
        auto applySnapshot = [this](PvpMatchSnapshotDto const& resolvedSnapshot) {
            log::info(
                "Versus overlay match for level {}: matchID={}, mode={}, scoringMode={}, targetScore={}, startingHp={}, finalizeAliveCount={}, status={}, context={}, roomName={}, participants={}, results={}",
                m_levelID, resolvedSnapshot.matchID, resolvedSnapshot.mode, resolvedSnapshot.scoringMode,
                resolvedSnapshot.targetScore, resolvedSnapshot.startingHp, resolvedSnapshot.finalizeAliveCount,
                resolvedSnapshot.status, resolvedSnapshot.context, resolvedSnapshot.roomName,
                resolvedSnapshot.participants.size(), resolvedSnapshot.results.size()
            );

            this->parseMatchSnapshot(resolvedSnapshot);
            this->refreshLabel();

            if (this->isReadyForRealtime()) {
                this->requestRealtimeToken();
            }
        };

        if (snapshot.context == "custom_room" && snapshot.matchID > 0) {
            PvpClient::getMatch(snapshot.matchID, [this, snapshot, applySnapshot](PvpMatchSnapshotDto const& detail,
                                                                                  web::WebResponse& detailRes) {
                if (m_cleanedUp) {
                    return;
                }

                auto resolvedSnapshot = snapshot;
                if (detailRes.ok() && detail.matchID > 0) {
                    resolvedSnapshot.scoringMode = detail.scoringMode;
                    resolvedSnapshot.targetScore = detail.targetScore;
                    resolvedSnapshot.startingHp = detail.startingHp;
                    resolvedSnapshot.finalizeAliveCount = detail.finalizeAliveCount;
                    if (!detail.mode.empty()) {
                        resolvedSnapshot.mode = detail.mode;
                    }
                    log::info(
                        "Resolved custom room overlay scoring metadata for match {}: scoringMode={}, targetScore={}, startingHp={}, finalizeAliveCount={}",
                        resolvedSnapshot.matchID, resolvedSnapshot.scoringMode, resolvedSnapshot.targetScore,
                        resolvedSnapshot.startingHp, resolvedSnapshot.finalizeAliveCount
                    );
                } else if (!detailRes.ok()) {
                    log::warn("Failed to resolve custom room overlay scoring metadata for match {}: HTTP {}",
                              snapshot.matchID, detailRes.code());
                }

                applySnapshot(resolvedSnapshot);
            });
            return;
        }

        applySnapshot(snapshot);
    });
}

void PvpOverlayService::parseMatchSnapshot(PvpMatchSnapshotDto const& snapshot) {
    m_matchID = snapshot.matchID;
    m_currentUid = snapshot.currentUid;
    m_mode = snapshot.mode == "platformer" ? "platformer" : "classic";
    m_scoringMode = normalizeScoringMode(snapshot.scoringMode);
    m_targetScore = std::max(0, snapshot.targetScore);
    m_startingHp = std::max(0, snapshot.startingHp);
    m_finalizeAliveCount = std::max(0, snapshot.finalizeAliveCount);
    m_context = snapshot.context == "custom_room" ? "custom_room" : "versus";
    m_roomName = snapshot.roomName;
    m_matchEndsAtEpoch = gdvn::utils::date::parseIsoEpochSeconds(snapshot.endsAt);
    m_lastCountdownSeconds = -1;
    auto status = snapshot.status;
    m_active = this->isActiveStatus(status);
    m_chatOpen = m_active || this->isCompletedStatus(status);
    m_chatGraceTimer = this->isCompletedStatus(status) ? CHAT_GRACE_SECONDS : -1.0f;

    m_self = {};
    m_opponent = {};
    m_players.clear();

    for (auto const& participant : snapshot.participants) {
        if (participant.valid) {
            PvpOverlayPlayerProgressModel player;
            player.uid = participant.uid;
            player.name = participant.name;
            player.progress = participant.progress;
            m_players.push_back(player);

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

    if (message.table == gdvn::consts::Table::PVP_MATCH_RESULTS) {
        this->handleResultRow(PvpMatchAdapter::playerProgressFromJson(message.row));
        this->refreshLabel();
        this->scheduleMessageRefresh();
    } else if (message.table == gdvn::consts::WebsocketEvent::MATCH_TABLE) {
        this->handleMatchRow(PvpMatchAdapter::matchRowFromJson(message.row));
        this->scheduleMessageRefresh();
    } else if (message.table == gdvn::consts::WebsocketEvent::MESSAGE_TABLE) {
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

    auto existing =
        std::find_if(m_players.begin(), m_players.end(),
                     [&row](PvpOverlayPlayerProgressModel const& item) { return item.uid == row.uid; });
    if (existing == m_players.end()) {
        PvpOverlayPlayerProgressModel player;
        player.uid = row.uid;
        player.name = row.name;
        player.progress = row.progress;
        m_players.push_back(player);
    } else {
        if (!row.name.empty()) {
            existing->name = row.name;
        }
        existing->progress = m_scoringMode == "hp" ? row.progress : std::max(existing->progress, row.progress);
    }

    if (!m_currentUid.empty() && row.uid == m_currentUid) {
        m_self.uid = row.uid;
        if (!row.name.empty()) {
            m_self.name = row.name;
        }
        m_self.progress = m_scoringMode == "hp" ? row.progress : std::max(m_self.progress, row.progress);
        return;
    }

    m_opponent.uid = row.uid;
    if (!row.name.empty()) {
        m_opponent.name = row.name;
    }
    m_opponent.progress = m_scoringMode == "hp" ? row.progress : std::max(m_opponent.progress, row.progress);
    if (m_scoringMode == "progress" && row.progress >= 100.0f && m_submitter) {
        m_submitter->flushDeathCount();
    }
}

void PvpOverlayService::handleMatchRow(PvpMatchRowDto const& row) {
    const bool wasActive = m_active;
    if (m_submitter && row.levelID > 0) {
        m_submitter->setMatchLevelID(row.levelID);
    }
    if (row.mode == "platformer") {
        m_mode = "platformer";
    }
    m_scoringMode = normalizeScoringMode(row.scoringMode);
    m_targetScore = std::max(0, row.targetScore);
    m_startingHp = std::max(0, row.startingHp);
    m_finalizeAliveCount = std::max(0, row.finalizeAliveCount);
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
        isProgressSystemMessage = kind == "progress" || kind == "hp_damage";
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
        auto nextLevelID = static_cast<int>(metadata.nextLevelID);
        if (m_submitter && nextLevelID > 0) {
            m_submitter->setMatchLevelID(nextLevelID);
        }
        m_self.playMode = "normal";
        m_opponent.playMode = "normal";
        for (auto& player : m_players) {
            player.playMode = "normal";
        }
        m_hideOverlayForLevelChange = nextLevelID > 0 && nextLevelID != m_levelID;
        if (m_hideOverlayForLevelChange) {
            this->setOverlayVisible(false);
        } else {
            this->refreshLabel();
        }
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

    for (auto& player : m_players) {
        if (player.uid == uid) {
            player.playMode = playMode;
        }
    }

    this->refreshLabel();
}

std::string PvpOverlayService::formatSystemMessage(PvpMatchSystemMetadataDto const& metadata) const {
    if (!metadata.valid) {
        return "Match update.";
    }

    auto kind = metadata.kind;
    if (kind == "hp_damage") {
        auto player = this->participantLabel(metadata.uid);
        return fmt::format("{} reached {} progress and dealt {} DMG.", player,
                           formatProgressForMode(metadata.progress, "classic"), formatProgress(metadata.damage));
    }

    if (kind == "progress") {
        auto progress = metadata.progress;
        auto mode = metadata.mode == "platformer" ? "platformer" : m_mode;
        auto scoringMode = metadata.scoringMode == "score" || metadata.scoringMode == "hp"
            ? metadata.scoringMode
            : m_scoringMode;
        auto targetScore = metadata.targetScore > 0 ? metadata.targetScore : m_targetScore;
        auto startingHp = metadata.startingHp > 0 ? metadata.startingHp : m_startingHp;
        auto player = this->participantLabel(metadata.uid);
        if (scoringMode == "score" && mode != "platformer") {
            return fmt::format("{} reached {} score.", player, formatScore(progress, targetScore));
        }
        if (scoringMode == "hp" && mode != "platformer") {
            return fmt::format("{} has {} remaining.", player, formatHp(progress, startingHp));
        }

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
                           this->participantLabel(winnerUid));
    }

    if (kind == "resignation") {
        auto resigning = this->participantLabel(metadata.resigningUid);
        auto winnerUid = metadata.winnerUid;
        if (winnerUid.empty()) {
            return fmt::format("{} resigned. The match ended.", resigning);
        }

        return fmt::format("{} resigned. {} won the match. Chat will remain open briefly.", resigning,
                           this->participantLabel(winnerUid));
    }

    if (kind == "level_change_requested") {
        return fmt::format("{} requested a level change. The level will change if both players agree.",
                           this->participantLabel(metadata.requesterUid));
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

std::string PvpOverlayService::participantLabel(std::string const& uid) const {
    if (!uid.empty() && !m_currentUid.empty() && uid == m_currentUid) {
        return "You";
    }

    if (this->isCustomRoomMatch()) {
        auto found = std::find_if(m_players.begin(), m_players.end(),
                                  [&uid](PvpOverlayPlayerProgressModel const& player) { return player.uid == uid; });
        if (found != m_players.end() && !found->name.empty()) {
            return found->name;
        }

        return "Player";
    }

    return "Opponent";
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

    return this->participantLabel(message.senderUid);
}

std::vector<PvpOverlayPlayerProgressModel> PvpOverlayService::sortedPlayers() const {
    auto players = m_players;
    std::sort(players.begin(), players.end(), [](auto const& left, auto const& right) {
        if (left.progress != right.progress) {
            return left.progress > right.progress;
        }

        return left.name < right.name;
    });
    return players;
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

    if (m_hideOverlayForLevelChange) {
        this->setOverlayVisible(false);
        return;
    }

    if (this->isCustomRoomMatch()) {
        auto title = "gdvn.net - Custom room";
        auto text = title + timerLine;
        auto players = this->sortedPlayers();
        for (auto const& player : players) {
            text += "\n";
            text += this->formatPlayerLabel(this->participantLabel(player.uid), player);
        }
        m_overlay->setText(text);
    } else {
        m_overlay->setText(fmt::format("gdvn.net - Versus{}\n{}\n{}", timerLine, formatPlayerLabel("You", m_self),
                                       formatPlayerLabel("Opponent", m_opponent)));
    }
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
        m_overlay->setVisible(visible && !m_hideOverlayForLevelChange);
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

bool PvpOverlayService::isCustomRoomMatch() const {
    return m_context == "custom_room";
}

std::string PvpOverlayService::formatProgressLabel(float progress) const {
    if (m_scoringMode == "score" && m_mode != "platformer") {
        return formatScore(progress, m_targetScore);
    }

    if (m_scoringMode == "hp" && m_mode != "platformer") {
        return formatHp(progress, m_startingHp);
    }

    return formatProgressForMode(progress, m_mode);
}
