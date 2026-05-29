#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>

using namespace geode::prelude;

class UpdateClient {
	static async::TaskHolder<web::WebResponse> m_get_holder;

public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void downloadLatest(Callback callback);
	static void getLatestRelease(Callback callback);
};
