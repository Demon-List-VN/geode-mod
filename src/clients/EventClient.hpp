#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class EventClient {
	static async::TaskHolder<web::WebResponse> m_put_holder;

public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void submitLevel(int levelID, float progress, std::string const& token, Callback callback);
};
