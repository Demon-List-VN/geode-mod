#pragma once

#include "PvpMessageDto.hpp"
#include <vector>

struct PvpMessagesResponseDto {
	bool valid = false;
	std::vector<PvpMessageDto> messages;
};
