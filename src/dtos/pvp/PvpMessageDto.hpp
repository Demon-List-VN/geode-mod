#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>
#include <string>

struct PvpMessageDto {
	bool valid = false;
	std::int64_t id = 0;
	std::string senderUid;
	std::string type;
	std::string content;
	bool senderAnonymous = false;
	matjson::Value metadata;
	matjson::Value rawJson;
};
