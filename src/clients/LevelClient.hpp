#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class LevelClient {
	static async::TaskHolder<web::WebResponse> m_get_holder;

public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void getLevel(int id, std::string const& token, Callback callback);
	static void getActivePvpMatch(int levelID, std::string const& token, Callback callback);
	static void getEventLevel(int levelID, std::string const& token, std::string const& type, Callback callback);
};
