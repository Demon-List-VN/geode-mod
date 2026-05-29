#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class ProgressClient {
	static async::TaskHolder<web::WebResponse> m_post_holder;

public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void submitHeatmap(size_t count, std::string const& token, Callback callback);
	static void submitDeathCount(int levelID, std::string const& count, bool completed, std::string const& token, Callback callback);
};
