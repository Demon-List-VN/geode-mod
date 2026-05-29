#pragma once

#include "LevelListItemAdapter.hpp"
#include "../dtos/level/LevelListDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class LevelListAdapter {
public:
	static LevelListDto fromJson(matjson::Value const& json) {
		LevelListDto dto;

		if (json["slug"].isString()) {
			dto.slug = json["slug"].asString().unwrapOrDefault();
		}

		if (json["title"].isString()) {
			dto.title = json["title"].asString().unwrapOrDefault();
		}

		if (json["mode"].isString()) {
			dto.mode = json["mode"].asString().unwrapOrDefault();
		}

		if (json["isOfficial"].isBool()) {
			dto.isOfficial = json["isOfficial"].asBool().unwrapOr(false);
		}

		if (json["starred"].isBool()) {
			dto.hasStarred = true;
			dto.starred = json["starred"].asBool().unwrapOr(false);
		}

		if (json["topEnabled"].isBool()) {
			dto.hasTopEnabled = true;
			dto.topEnabled = json["topEnabled"].asBool().unwrapOr(false);
		}

		if (json["item"].isObject()) {
			dto.item = LevelListItemAdapter::fromJson(json["item"]);
		}

		return dto;
	}
};

}
