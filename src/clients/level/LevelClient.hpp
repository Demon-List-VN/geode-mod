#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class LevelClient {
public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void getLevel(int id, Callback callback);
	static void getActivePvpMatch(int levelID, Callback callback);
};
