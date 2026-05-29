#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

using namespace geode::prelude;

class AuthClient {
	static async::TaskHolder<web::WebResponse> m_post_holder;
	static async::TaskHolder<web::WebResponse> m_get_holder;

public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void requestOTP(Callback callback);
	static void checkOTP(std::string const& code, Callback callback);
	static void logout(std::string const& token, Callback callback);
	static void checkMe(std::string const& token, std::string const& modVersion, Callback callback);
	static void getRealtimeToken(std::string const& token, Callback callback);
};
