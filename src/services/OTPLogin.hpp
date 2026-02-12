#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class OTPLogin {
	static async::TaskHolder<web::WebResponse> m_post_holder, m_get_holder;
public:
	static void showLoginPrompt();
	static void requestOTP();
	static void checkOTP(std::string code);
	static void logout();
	static bool isLoggedIn();
};
