#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>

#include "../dtos/LevelInfoResponseDto.hpp"

using namespace geode::prelude;

class LevelService {
public:
	using GetLevelCallback = std::function<void(
		LevelInfoResponseDto const& level,
		web::WebResponse& response
	)>;

	static void getLevel(int id, GetLevelCallback callback);
};
