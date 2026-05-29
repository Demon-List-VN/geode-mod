#pragma once

#include "../dtos/pvp/ActivePvpMatchResponseDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class ActivePvpMatchResponseAdapter {
  public:
    static ActivePvpMatchResponseDto fromJson(matjson::Value const& json) {
        ActivePvpMatchResponseDto dto;
        dto.rawJson = json;

        if (json["matchId"].isNumber()) {
            dto.valid = true;
            dto.matchID = static_cast<int>(json["matchId"].asDouble().unwrapOr(0.0));
        }

        if (json["mode"].isString()) {
            dto.mode = json["mode"].asString().unwrapOrDefault();
        }

        return dto;
    }
};

} // namespace gdvn::adapters
