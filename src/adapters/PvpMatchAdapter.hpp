#pragma once

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
        dto.currentUid = getString(json, "currentUid");
        dto.mode = getString(json, "mode") == "platformer" ? "platformer" : "classic";
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
        dto.event = getString(json, "event");

        if (dto.event == "phx_reply") {
            dto.replyOk = getString(json["payload"], "status") == "ok";
            return dto;
        }

        if (dto.event != "postgres_changes") {
            return dto;
        }

        auto const& payload = json["payload"];
        auto const& data = payload["data"];
        dto.table = getString(data, "table");
        if (dto.table.empty()) {
            dto.table = getString(payload, "table");
        }

        dto.row = realtimeRecord(payload);
        if (dto.table.empty() && dto.row["matchId"].isNumber() && dto.row["content"].isString()) {
            dto.table = "pvpMatchMessages";
        }

        dto.rowMatchID = getInteger(dto.row, "matchId");
        dto.rowID = getInteger(dto.row, "id");
        return dto;
    }

    static PvpMatchPlayerProgressDto playerProgressFromJson(matjson::Value const& json) {
        PvpMatchPlayerProgressDto dto;
        dto.uid = getString(json, "uid");
        dto.progress = getNumber(json, "progress");
        dto.valid = !dto.uid.empty();
        return dto;
    }

    static PvpMatchRowDto matchRowFromJson(matjson::Value const& json) {
        PvpMatchRowDto dto;
        dto.mode = getString(json, "mode");
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
        dto.mode = getString(json, "mode");
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

    static PvpMatchPlayerProgressDto participantFromJson(matjson::Value const& json) {
        auto dto = playerProgressFromJson(json);

        if (!json["result"].isNull() && json["result"].isObject()) {
            dto.progress = std::max(dto.progress, getNumber(json["result"], "progress"));
        }

        return dto;
    }

    static matjson::Value const& realtimeRecord(matjson::Value const& payload) {
        auto const& data = payload["data"];

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
