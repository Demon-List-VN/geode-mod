#include "AntiCheatService.hpp"

#include "../../consts/ConfigConst.hpp"

#include <optional>
#include <string>
#include <string_view>

using namespace geode::prelude;

namespace {
template <class Function>
void withConfigBasedCheatChecks(Function&& function) {
    if constexpr (gdvn::config::ENABLE_CONFIG_BASED_CHEAT_CHECKS) {
        function();
    }
}
} // namespace

AntiCheatService::AntiCheatService() {
}

AntiCheatService::AntiCheatService(PlayLayer* playLayer) : playLayer(playLayer) {
}

std::optional<std::string_view> AntiCheatService::getCheatedReason() const {
    std::optional<std::string_view> configBasedReason;
    withConfigBasedCheatChecks([&] {
        if (eclipseAntiCheat.isCheated()) {
            configBasedReason = eclipseAntiCheat.getCheatReason();
            return;
        }

        if (qolModAntiCheat.isCheated()) {
            configBasedReason = qolModAntiCheat.getCheatReason();
            return;
        }

        if (prismAntiCheat.isCheated()) {
            configBasedReason = prismAntiCheat.getCheatReason();
            return;
        }

        if (openHackAntiCheat.isCheated()) {
            configBasedReason = openHackAntiCheat.getCheatReason();
        }
    });

    if (configBasedReason) {
        return configBasedReason;
    }

    if (startPositionAntiCheat.isCheated()) {
        return startPositionAntiCheat.getCheatReason();
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
    withConfigBasedCheatChecks([&] {
        eclipseAntiCheat.reset(playLayer);
        qolModAntiCheat.reset(playLayer);
        prismAntiCheat.reset(playLayer);
        openHackAntiCheat.reset(playLayer);
    });
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
    withConfigBasedCheatChecks([&] {
        eclipseAntiCheat.onUpdate(dt);
        qolModAntiCheat.onUpdate(dt);
        prismAntiCheat.onUpdate(dt);
        openHackAntiCheat.onUpdate(dt);
    });
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
