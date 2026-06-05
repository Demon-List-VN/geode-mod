#pragma once

#include "../dtos/pvp/PvpPowerupCastResponseDto.hpp"
#include "../dtos/pvp/PvpPowerupStateDto.hpp"
#include <Geode/Geode.hpp>
#include <cstdint>
#include <vector>

class PvpPowerupAdapter {
  public:
    static PvpPowerupStateDto stateFromJson(matjson::Value const& json) {
        PvpPowerupStateDto dto;
        if (!json.isObject()) {
            return dto;
        }

        dto.valid = true;
        dto.matchID = static_cast<int>(getInteger(json, "matchId"));
        dto.uid = getString(json, "uid");
        dto.mana = static_cast<int>(getInteger(json, "mana"));
        dto.maxMana = static_cast<int>(getInteger(json, "maxMana"));
        if (dto.maxMana <= 0) {
            dto.maxMana = 100;
        }
        dto.shieldExpiresAt = getString(json, "shieldExpiresAt");
        dto.shieldCharges = static_cast<int>(getInteger(json, "shieldCharges"));
        dto.shieldActive = getBool(json, "shieldActive");

        if (json["skills"].isArray()) {
            for (auto const& skillJson : json["skills"].asArray().unwrap()) {
                auto skill = skillFromJson(skillJson);
                if (!skill.skill.empty()) {
                    dto.skills.push_back(skill);
                }
            }
        }

        if (dto.skills.empty()) {
            dto.skills = defaultSkills();
        }

        return dto;
    }

    static PvpPowerupCastResponseDto castResponseFromJson(matjson::Value const& json) {
        PvpPowerupCastResponseDto dto;
        if (!json.isObject()) {
            return dto;
        }

        dto.valid = true;
        dto.skill = getString(json, "skill");
        dto.targetUid = getString(json, "targetUid");
        dto.blocked = getBool(json, "blocked");
        dto.state = stateFromJson(json["state"]);
        return dto;
    }

    static std::vector<PvpPowerupSkillDto> defaultSkills() {
        return {
            {"flashbang", 60, 1000, "flashbang", true},
            {"invisible", 100, 2000, "invisible", true},
            {"shield", 50, 20000, "shield", false},
            {"pause", 40, 0, "pause", true},
            {"double_click", 50, 10000, "double_click", true},
            {"force_reset", 100, 0, "force_reset", true}
        };
    }

  private:
    static PvpPowerupSkillDto skillFromJson(matjson::Value const& json) {
        PvpPowerupSkillDto dto;
        if (!json.isObject()) {
            return dto;
        }

        dto.skill = getString(json, "skill");
        dto.cost = static_cast<int>(getInteger(json, "cost"));
        dto.durationMs = static_cast<int>(getInteger(json, "durationMs"));
        dto.effect = getString(json, "effect");
        dto.harmful = getBool(json, "harmful");
        return dto;
    }

    static std::string getString(matjson::Value const& json, char const* key) {
        if (!json[key].isString()) {
            return "";
        }

        return json[key].asString().unwrapOrDefault();
    }

    static std::int64_t getInteger(matjson::Value const& json, char const* key) {
        if (!json[key].isNumber()) {
            return 0;
        }

        return static_cast<std::int64_t>(json[key].asDouble().unwrapOr(0.0));
    }

    static bool getBool(matjson::Value const& json, char const* key) {
        return json[key].asBool().unwrapOr(false);
    }
};
