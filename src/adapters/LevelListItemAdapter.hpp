#pragma once

#include "../dtos/LevelListItemDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class LevelListItemAdapter {
public:
	static LevelListItemDto fromJson(matjson::Value const& json) {
		LevelListItemDto dto;

		if (json["position"].isNumber()) {
			dto.position = json["position"].asDouble().unwrapOr(0.0);
		}

		if (json["rating"].isNumber()) {
			dto.rating = json["rating"].asDouble().unwrapOr(0.0);
		}

		return dto;
	}
};

}
