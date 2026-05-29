#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>

#include "../models/LevelModels.hpp"

using namespace geode::prelude;

class LevelService {
public:
	using GetLevelCallback = std::function<void(
		gdvn::models::LevelInfoResponseModel const& level,
		web::WebResponse& response
	)>;

	static void getLevel(int id, GetLevelCallback callback);
};
