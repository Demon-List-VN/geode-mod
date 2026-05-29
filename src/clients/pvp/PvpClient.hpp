#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <cstdint>
#include <functional>
#include <string>

using namespace geode::prelude;

class PvpClient {
public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void putPlayMode(int matchID, std::string const& playMode, Callback callback);
	static void putProgress(int matchID, float progress, bool completed, Callback callback);
	static void postDeathCount(int matchID, std::string const& count, Callback callback);
	static void getMessages(int matchID, std::int64_t afterID, int limit, Callback callback);
	static void postMessage(int matchID, std::string const& content, Callback callback);
};
