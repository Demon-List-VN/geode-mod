#pragma once

#include "../dtos/pvp/PvpMessageDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class PvpMessageAdapter {
public:
	static PvpMessageDto fromJson(matjson::Value const& json) {
		PvpMessageDto dto;
		dto.rawJson = json;

		if (json["id"].isNumber()) {
			dto.id = static_cast<std::int64_t>(json["id"].asDouble().unwrapOr(0.0));
		}

		if (json["senderUid"].isString()) {
			dto.senderUid = json["senderUid"].asString().unwrapOrDefault();
		}

		if (json["type"].isString()) {
			dto.type = json["type"].asString().unwrapOrDefault();
		}

		if (json["content"].isString()) {
			dto.content = json["content"].asString().unwrapOrDefault();
		}

		dto.senderAnonymous =
			json["senderAnonymous"].asBool().unwrapOr(false) ||
			json["sender_anonymous"].asBool().unwrapOr(false);
		dto.metadata = json["metadata"];
		dto.valid = dto.id > 0 || !dto.content.empty() || dto.type == "system";

		return dto;
	}
};

}
