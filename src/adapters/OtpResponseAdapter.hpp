#pragma once

#include "../dtos/OtpResponseDto.hpp"
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

}
