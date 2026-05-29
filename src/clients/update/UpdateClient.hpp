#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>

using namespace geode::prelude;

class UpdateClient {
public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void getLatestDownload(Callback callback);
	static void getLatestRelease(Callback callback);
};
