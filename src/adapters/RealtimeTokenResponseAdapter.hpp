#pragma once

#include "../dtos/pvp/RealtimeTokenResponseDto.hpp"
#include <Geode/Geode.hpp>

class RealtimeTokenResponseAdapter {
  public:
    static RealtimeTokenResponseDto fromJson(matjson::Value const& json) {
        RealtimeTokenResponseDto dto;

        if (json["supabaseUrl"].isString()) {
            dto.supabaseUrl = json["supabaseUrl"].asString().unwrapOrDefault();
        }

        if (json["anonKey"].isString()) {
            dto.anonKey = json["anonKey"].asString().unwrapOrDefault();
        }

        if (json["accessToken"].isString()) {
            dto.accessToken = json["accessToken"].asString().unwrapOrDefault();
        }

        if (json["expiresAt"].isNumber()) {
            dto.expiresAt = static_cast<std::int64_t>(json["expiresAt"].asDouble().unwrapOr(0.0));
        }

        dto.valid = !dto.supabaseUrl.empty() && !dto.anonKey.empty() && !dto.accessToken.empty();
        return dto;
    }
};

