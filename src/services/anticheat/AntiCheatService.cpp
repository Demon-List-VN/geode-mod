#include "AntiCheatService.hpp"

#include "../../config.hpp"

#include <optional>
#include <string>
#include <string_view>

using namespace geode::prelude;

AntiCheatService::AntiCheatService() {
}

AntiCheatService::AntiCheatService(PlayLayer* playLayer) : playLayer(playLayer) {
}

std::optional<std::string_view> AntiCheatService::getCheatedReason() const {
    if (startPositionAntiCheat.isCheated()) {
        return startPositionAntiCheat.getCheatReason();
    }

    if (eclipseAntiCheat.isCheated()) {
        return eclipseAntiCheat.getCheatReason();
    }

    if constexpr (gdvn::config::ENABLE_CONFIG_BASED_CHEAT_CHECKS) {
        if (qolModAntiCheat.isCheated()) {
            return qolModAntiCheat.getCheatReason();
        }

        if (prismAntiCheat.isCheated()) {
            return prismAntiCheat.getCheatReason();
        }
    }

    if (openHackAntiCheat.isCheated()) {
        return openHackAntiCheat.getCheatReason();
    }

    if (damageBypassAntiCheat.isCheated()) {
        return damageBypassAntiCheat.getCheatReason();
    }

    if (noclipAntiCheat.isCheated()) {
        return noclipAntiCheat.getCheatReason();
    }

    return std::nullopt;
}

void AntiCheatService::reset(PlayLayer* playLayer) {
    this->playLayer = playLayer;
    reportedCheat = false;
    reportedCheatReason.clear();

    startPositionAntiCheat.reset(playLayer);
    eclipseAntiCheat.reset(playLayer);
    qolModAntiCheat.reset(playLayer);
    prismAntiCheat.reset(playLayer);
    openHackAntiCheat.reset(playLayer);
    damageBypassAntiCheat.reset(playLayer);
    noclipAntiCheat.reset(playLayer);

    markRunCheatedIfNeeded();
}

void AntiCheatService::markRunCheated(std::string const& reason) const {
    if (reportedCheat) {
        return;
    }

    reportedCheat = true;
    reportedCheatReason = reason;

    if (playLayer && playLayer->m_level) {
        log::warn("Run marked as cheated on level {}: {}", playLayer->m_level->m_levelID.value(), reason);
    } else {
        log::warn("Run marked as cheated: {}", reason);
    }
}

void AntiCheatService::markRunCheatedIfNeeded() const {
    if (auto reason = getCheatedReason()) {
        markRunCheated(std::string(*reason));
    }
}

void AntiCheatService::onUpdate(float dt) {
    startPositionAntiCheat.onUpdate(dt);
    eclipseAntiCheat.onUpdate(dt);
    qolModAntiCheat.onUpdate(dt);
    prismAntiCheat.onUpdate(dt);
    openHackAntiCheat.onUpdate(dt);
    damageBypassAntiCheat.onUpdate(dt);
    noclipAntiCheat.onUpdate(dt);

    markRunCheatedIfNeeded();
}

void AntiCheatService::onPlayerDestroyed(PlayerObject* player) {
    noclipAntiCheat.onPlayerDestroyed(player);
    markRunCheatedIfNeeded();
}

bool AntiCheatService::isCheated() const {
    markRunCheatedIfNeeded();
    return getCheatedReason().has_value();
}

std::string_view AntiCheatService::getCheatReason() const {
    return reportedCheatReason;
}
