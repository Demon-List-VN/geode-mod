#pragma once

#include "LevelListDto.hpp"
#include <vector>

struct LevelInfoResponseDto {
	bool valid = false;
	std::vector<LevelListDto> lists;
};

