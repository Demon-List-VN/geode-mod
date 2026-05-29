#pragma once

#include "../dtos/OtpGrantResponseDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class OtpGrantResponseAdapter {
public:
	static OtpGrantResponseDto fromJson(matjson::Value const& json) {
		OtpGrantResponseDto dto;
		dto.granted = json["granted"].asBool().unwrapOr(false);

		if (!dto.granted) {
			dto.valid = true;
			return dto;
		}

		if (json["key"].isString() && json["player"].isString()) {
			dto.valid = true;
			dto.key = json["key"].asString().unwrapOrDefault();
			dto.player = json["player"].asString().unwrapOrDefault();
		}

		return dto;
	}
};

}
