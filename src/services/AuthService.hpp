#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class AuthService {
	static async::TaskHolder<web::WebResponse> m_post_holder, m_get_holder;
public:
	static void login();
    static void logout();
    static void check();
    static std::string getToken();
	static void requestOTP();
	static void checkOTP(std::string code);
	static bool isLoggedIn();
};
