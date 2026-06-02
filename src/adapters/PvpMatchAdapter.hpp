#pragma once

#include "../consts/WebsocketEventConst.hpp"
#include "../dtos/pvp/match/PvpMatchPlayerProgressDto.hpp"
#include "../dtos/pvp/match/PvpMatchRealtimeMessageDto.hpp"
#include "../dtos/pvp/match/PvpMatchRowDto.hpp"
#include "../dtos/pvp/match/PvpMatchSnapshotDto.hpp"
#include "../dtos/pvp/match/PvpMatchSystemMetadataDto.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cstdint>
#include <string>

class PvpMatchAdapter {
  public:
    static PvpMatchSnapshotDto matchSnapshotFromJson(matjson::Value const& json) {
        PvpMatchSnapshotDto dto;
        dto.matchID = static_cast<int>(getNumber(json, "matchId"));
        if (dto.matchID <= 0) {
            dto.matchID = static_cast<int>(getNumber(json, "id"));
        }
        dto.currentUid = getString(json, "currentUid");
        dto.mode = getString(json, "mode") == "platformer" ? "platformer" : "classic";
        dto.scoringMode = getScoringMode(json);
        dto.targetScore = static_cast<int>(getTargetScore(json));
        dto.startingHp = static_cast<int>(getStartingHp(json));
        dto.finalizeAliveCount = static_cast<int>(getFinalizeAliveCount(json));
        dto.context = getString(json, "context");
        dto.roomName = getString(json, "roomName");
        if (dto.roomName.empty() && json["room"].isObject()) {
            dto.roomName = getString(json["room"], "name");
        }
        dto.endsAt = getString(json, "endsAt");
        dto.status = getString(json, "status");

        if (json["participants"].isArray()) {
            for (auto const& participant : json["participants"].asArray().unwrap()) {
                dto.participants.push_back(participantFromJson(participant));
            }
        }

        if (json["results"].isArray()) {
            for (auto const& result : json["results"].asArray().unwrap()) {
                dto.results.push_back(playerProgressFromJson(result));
            }
        }

        return dto;
    }

    static PvpMatchRealtimeMessageDto realtimeMessageFromString(std::string const& message) {
        auto json = matjson::parse(message);
        if (!json) {
            return {};
        }

        return realtimeMessageFromJson(json.unwrap());
    }

    static PvpMatchRealtimeMessageDto realtimeMessageFromJson(matjson::Value const& json) {
        PvpMatchRealtimeMessageDto dto;
        dto.valid = true;
        dto.event = getString(json, gdvn::consts::WebsocketEvent::EVENT);

        if (dto.event == gdvn::consts::WebsocketEvent::PHX_REPLY) {
            dto.replyOk =
                getString(json[gdvn::consts::WebsocketEvent::PAYLOAD], gdvn::consts::WebsocketEvent::STATUS) ==
                "ok";
            return dto;
        }

        if (dto.event != gdvn::consts::WebsocketEvent::POSTGRES_CHANGES) {
            return dto;
        }

        auto const& payload = json[gdvn::consts::WebsocketEvent::PAYLOAD];
        auto const& data = payload[gdvn::consts::WebsocketEvent::DATA];
        dto.table = getString(data, gdvn::consts::WebsocketEvent::TABLE);
        if (dto.table.empty()) {
            dto.table = getString(payload, gdvn::consts::WebsocketEvent::TABLE);
        }

        dto.row = realtimeRecord(payload);
        if (dto.table.empty() && dto.row["matchId"].isNumber() && dto.row["content"].isString()) {
            dto.table = gdvn::consts::WebsocketEvent::MESSAGE_TABLE;
        }

        dto.rowMatchID = getInteger(dto.row, "matchId");
        dto.rowID = getInteger(dto.row, "id");
        return dto;
    }

    static PvpMatchPlayerProgressDto playerProgressFromJson(matjson::Value const& json) {
        PvpMatchPlayerProgressDto dto;
        dto.uid = getString(json, "uid");
        dto.name = getString(json, "name");
        if (dto.name.empty()) {
            dto.name = getString(json, "displayName");
        }
        if (dto.name.empty() && json["player"].isObject()) {
            dto.name = getString(json["player"], "name");
        }
        if (dto.name.empty() && json["players"].isObject()) {
            dto.name = getString(json["players"], "name");
        }
        dto.progress = getNumber(json, "progress");
        dto.valid = !dto.uid.empty();
        return dto;
    }

    static PvpMatchRowDto matchRowFromJson(matjson::Value const& json) {
        PvpMatchRowDto dto;
        dto.levelID = static_cast<int>(getInteger(json, "levelId"));
        dto.mode = getString(json, "mode");
        dto.scoringMode = getScoringMode(json);
        dto.targetScore = static_cast<int>(getTargetScore(json));
        dto.startingHp = static_cast<int>(getStartingHp(json));
        dto.finalizeAliveCount = static_cast<int>(getFinalizeAliveCount(json));
        dto.endsAt = getString(json, "endsAt");
        dto.status = getString(json, "status");
        return dto;
    }

    static PvpMatchSystemMetadataDto systemMetadataFromJson(matjson::Value const& json) {
        PvpMatchSystemMetadataDto dto;
        if (!json.isObject()) {
            return dto;
        }

        dto.valid = true;
        dto.kind = getString(json, "kind");
        dto.uid = getString(json, "uid");
        dto.playMode = getString(json, "playMode");
        dto.progress = getNumber(json, "progress");
        dto.damage = getNumber(json, "damage");
        dto.mode = getString(json, "mode");
        dto.scoringMode = getScoringMode(json);
        dto.targetScore = static_cast<int>(getTargetScore(json));
        dto.startingHp = static_cast<int>(getStartingHp(json));
        dto.finalizeAliveCount = static_cast<int>(getFinalizeAliveCount(json));
        dto.winnerUid = getString(json, "winnerUid");
        dto.resigningUid = getString(json, "resigningUid");
        dto.requesterUid = getString(json, "requesterUid");
        dto.nextLevelID = getInteger(json, "nextLevelId");
        return dto;
    }

  private:
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

    static float getNumber(matjson::Value const& json, char const* key) {
        if (!json[key].isNumber()) {
            return 0.0f;
        }

        return static_cast<float>(json[key].asDouble().unwrapOr(0.0));
    }

    static std::string getScoringMode(matjson::Value const& json) {
        auto mode = getString(json, "scoringMode");
        if (mode.empty()) {
            mode = getString(json, "scoring_mode");
        }
        if (mode.empty()) {
            mode = getString(json, "scoreMode");
        }
        if (mode.empty() && json["match"].isObject()) {
            mode = getScoringMode(json["match"]);
        }
        if (mode.empty() && json["room"].isObject()) {
            mode = getScoringMode(json["room"]);
        }

        return mode == "score" || mode == "hp" ? mode : "progress";
    }

    static std::int64_t getTargetScore(matjson::Value const& json) {
        auto value = getInteger(json, "targetScore");
        if (value <= 0) {
            value = getInteger(json, "target_score");
        }
        if (value <= 0 && json["match"].isObject()) {
            value = getTargetScore(json["match"]);
        }
        if (value <= 0 && json["room"].isObject()) {
            value = getTargetScore(json["room"]);
        }

        return value;
    }

    static std::int64_t getStartingHp(matjson::Value const& json) {
        auto value = getInteger(json, "startingHp");
        if (value <= 0) {
            value = getInteger(json, "starting_hp");
        }
        if (value <= 0 && json["match"].isObject()) {
            value = getStartingHp(json["match"]);
        }
        if (value <= 0 && json["room"].isObject()) {
            value = getStartingHp(json["room"]);
        }

        return value;
    }

    static std::int64_t getFinalizeAliveCount(matjson::Value const& json) {
        auto value = getInteger(json, "finalizeAliveCount");
        if (value <= 0) {
            value = getInteger(json, "finalize_alive_count");
        }
        if (value <= 0 && json["match"].isObject()) {
            value = getFinalizeAliveCount(json["match"]);
        }
        if (value <= 0 && json["room"].isObject()) {
            value = getFinalizeAliveCount(json["room"]);
        }

        return value;
    }

    static PvpMatchPlayerProgressDto participantFromJson(matjson::Value const& json) {
        auto dto = playerProgressFromJson(json);

        if (!json["result"].isNull() && json["result"].isObject()) {
            dto.progress = std::max(dto.progress, getNumber(json["result"], "progress"));
        }
        if (dto.name.empty() && json["player"].isObject()) {
            dto.name = getString(json["player"], "name");
        }
        if (dto.name.empty() && json["players"].isObject()) {
            dto.name = getString(json["players"], "name");
        }

        return dto;
    }

    static matjson::Value const& realtimeRecord(matjson::Value const& payload) {
        auto const& data = payload[gdvn::consts::WebsocketEvent::DATA];

        if (data["record"].isObject()) {
            return data["record"];
        }

        if (payload["record"].isObject()) {
            return payload["record"];
        }

        if (payload["new"].isObject()) {
            return payload["new"];
        }

        return payload;
    }
};
