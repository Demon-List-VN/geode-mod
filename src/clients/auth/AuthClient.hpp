#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class AuthClient {
public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void postOTP(Callback callback);
	static void getOTP(std::string const& code, Callback callback);
	static void deleteAPIKey(Callback callback);
	static void getMe(Callback callback);
	static void getRealtimeToken(Callback callback);
};
