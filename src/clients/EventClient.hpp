#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class EventClient {
public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void putLevel(int levelID, float progress, Callback callback);
};
