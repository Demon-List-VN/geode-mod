#pragma once

#include "../dtos/pvp/ActivePvpMatchResponseDto.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cstdint>

class ActivePvpMatchResponseAdapter {
  public:
    static ActivePvpMatchResponseDto fromJson(matjson::Value const& json) {
        ActivePvpMatchResponseDto dto;
        dto.rawJson = json;

        if (json["matchId"].isNumber()) {
            dto.valid = true;
            dto.matchID = static_cast<int>(json["matchId"].asDouble().unwrapOr(0.0));
        }

        if (json["levelId"].isNumber()) {
            dto.levelID = static_cast<int>(json["levelId"].asDouble().unwrapOr(0.0));
        }

        dto.currentUid = readString(json, "currentUid");
        dto.bestProgress = readBestProgress(json, dto.currentUid);

        if (json["mode"].isString()) {
            dto.mode = json["mode"].asString().unwrapOrDefault();
        }

        dto.scoringMode = readScoringMode(json);
        dto.targetScore = static_cast<int>(readTargetScore(json));
        dto.startingHp = static_cast<int>(readStartingHp(json));
        dto.finalizeAliveCount = static_cast<int>(readFinalizeAliveCount(json));

        if (json["status"].isString()) {
            dto.status = json["status"].asString().unwrapOrDefault();
        }

        if (json["context"].isString()) {
            dto.context = json["context"].asString().unwrapOrDefault();
        }

        if (json["roomName"].isString()) {
            dto.roomName = json["roomName"].asString().unwrapOrDefault();
        }

        return dto;
    }

  private:
    static std::string readString(matjson::Value const& json, char const* key) {
        if (!json[key].isString()) {
            return "";
        }

        return json[key].asString().unwrapOrDefault();
    }

    static std::int64_t readInteger(matjson::Value const& json, char const* key) {
        if (!json[key].isNumber()) {
            return 0;
        }

        return static_cast<std::int64_t>(json[key].asDouble().unwrapOr(0.0));
    }

    static float readNumber(matjson::Value const& json, char const* key) {
        if (!json[key].isNumber()) {
            return 0.0f;
        }

        return static_cast<float>(json[key].asDouble().unwrapOr(0.0));
    }

    static float readBestProgress(matjson::Value const& json, std::string const& currentUid) {
        if (currentUid.empty()) {
            return 0.0f;
        }

        float best = 0.0f;
        auto readRows = [&best, &currentUid](matjson::Value const& rows) {
            if (!rows.isArray()) {
                return;
            }

            for (auto const& row : rows.asArray().unwrap()) {
                if (readString(row, "uid") != currentUid) {
                    continue;
                }

                best = std::max(best, readNumber(row, "progress"));
                if (row["result"].isObject()) {
                    best = std::max(best, readNumber(row["result"], "progress"));
                }
            }
        };

        readRows(json["participants"]);
        readRows(json["results"]);
        return best;
    }

    static std::string readScoringMode(matjson::Value const& json) {
        auto mode = readString(json, "scoringMode");
        if (mode.empty()) {
            mode = readString(json, "scoring_mode");
        }
        if (mode.empty()) {
            mode = readString(json, "scoreMode");
        }
        if (mode.empty() && json["match"].isObject()) {
            mode = readScoringMode(json["match"]);
        }
        if (mode.empty() && json["room"].isObject()) {
            mode = readScoringMode(json["room"]);
        }

        return mode == "score" || mode == "hp" || mode == "powerup" ? mode : "progress";
    }

    static std::int64_t readTargetScore(matjson::Value const& json) {
        auto value = readInteger(json, "targetScore");
        if (value <= 0) {
            value = readInteger(json, "target_score");
        }
        if (value <= 0 && json["match"].isObject()) {
            value = readTargetScore(json["match"]);
        }
        if (value <= 0 && json["room"].isObject()) {
            value = readTargetScore(json["room"]);
        }

        return value;
    }

    static std::int64_t readStartingHp(matjson::Value const& json) {
        auto value = readInteger(json, "startingHp");
        if (value <= 0) {
            value = readInteger(json, "starting_hp");
        }
        if (value <= 0 && json["match"].isObject()) {
            value = readStartingHp(json["match"]);
        }
        if (value <= 0 && json["room"].isObject()) {
            value = readStartingHp(json["room"]);
        }

        return value;
    }

    static std::int64_t readFinalizeAliveCount(matjson::Value const& json) {
        auto value = readInteger(json, "finalizeAliveCount");
        if (value <= 0) {
            value = readInteger(json, "finalize_alive_count");
        }
        if (value <= 0 && json["match"].isObject()) {
            value = readFinalizeAliveCount(json["match"]);
        }
        if (value <= 0 && json["room"].isObject()) {
            value = readFinalizeAliveCount(json["room"]);
        }

        return value;
    }
};
