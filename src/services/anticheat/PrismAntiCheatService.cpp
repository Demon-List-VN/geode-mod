#include "PrismAntiCheatService.hpp"

#include <Geode/Geode.hpp>

#include <array>
#include <cmath>
#include <optional>
#include <string_view>
#include <vector>

namespace {

bool differsFrom(float value, float expected) {
    constexpr float FLOAT_EPSILON = 0.001f;
    return std::abs(value - expected) > FLOAT_EPSILON;
}

std::optional<matjson::Value> getSavedValue(std::vector<matjson::Value> const& values, std::string_view name) {
    for (auto const& item : values) {
        if (!item.isObject()) {
            continue;
        }

        if (item["name"].asString().unwrapOrDefault() == name) {
            return item["value"];
        }
    }

    return std::nullopt;
}

bool getSavedBool(std::vector<matjson::Value> const& values, std::string_view name) {
    auto value = getSavedValue(values, name);
    return value && value->asBool().unwrapOr(false);
}

float getSavedFloat(std::vector<matjson::Value> const& values, std::string_view name, float defaultValue) {
    auto value = getSavedValue(values, name);
    if (!value) {
        return defaultValue;
    }

    return static_cast<float>(value->asDouble().unwrapOr(defaultValue));
}

} // namespace

void PrismAntiCheatService::reset(PlayLayer*) {
}

void PrismAntiCheatService::onUpdate(float) {
}

bool PrismAntiCheatService::isCheated() const {
    auto mod = geode::Loader::get()->getLoadedMod("firee.prism");
    if (!mod) {
        return false;
    }

    auto values = mod->getSavedValue<std::vector<matjson::Value>>("values");

    static constexpr std::array boolCheats = {
        "Safe Mode",
        "Anticheat Bypass",
        "Noclip",
        "Instant Complete",
        "No Spikes",
        "No Hitbox",
        "No Solids",
        "No Rotate",
        "Hide Player",
        "Freeze Player",
        "Jump Hack",
        "Layout Mode",
        "Show Hidden Objects",
        "Hide Level",
        "Change Gravity",
        "Force Platformer Mode",
        "No Mirror Transition",
        "Instant Mirror Portal",
        "Show Hitboxes",
        "No Shaders",
        "Playback",
        "Disable Camera Effects",
        "Auto Clicker",
    };

    for (auto cheat : boolCheats) {
        if (getSavedBool(values, cheat)) {
            return true;
        }
    }

    return differsFrom(getSavedFloat(values, "TPS Bypass", 240.0f), 240.0f);
}

std::string_view PrismAntiCheatService::getCheatReason() const {
    return "Prism config";
}
