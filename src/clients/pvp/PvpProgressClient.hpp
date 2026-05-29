#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class PvpProgressClient {
public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void postHeatmap(size_t count, Callback callback);
	static void postDeathCount(int levelID, std::string const& count, bool completed, Callback callback);
};
