#pragma once

#include "../dtos/auth/OtpResponseDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class OtpResponseAdapter {
  public:
    static OtpResponseDto fromJson(matjson::Value const& json) {
        OtpResponseDto dto;

        if (json["code"].isString()) {
            dto.valid = true;
            dto.code = json["code"].asString().unwrapOrDefault();
        }

        return dto;
    }
};

} // namespace gdvn::adapters
