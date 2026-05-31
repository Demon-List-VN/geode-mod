#pragma once

#include "../dtos/pvp/ActivePvpMatchResponseDto.hpp"
#include <Geode/Geode.hpp>

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

        if (json["mode"].isString()) {
            dto.mode = json["mode"].asString().unwrapOrDefault();
        }

        if (json["context"].isString()) {
            dto.context = json["context"].asString().unwrapOrDefault();
        }

        if (json["roomName"].isString()) {
            dto.roomName = json["roomName"].asString().unwrapOrDefault();
        }

        return dto;
    }
};
