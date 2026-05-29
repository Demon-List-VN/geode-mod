#pragma once

#include "../dtos/auth/AuthMeResponseDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class AuthMeResponseAdapter {
public:
	static AuthMeResponseDto fromJson(matjson::Value const& json) {
		AuthMeResponseDto dto;

		if (json["name"].isString()) {
			dto.valid = true;
			dto.name = json["name"].asString().unwrapOrDefault();
		}

		return dto;
	}
};

}
