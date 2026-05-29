#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class AuthService {
public:
	static void login();
    static void logout();
    static void check();
    static std::string getToken();
    static std::string getPlayerName();
	static void requestOTP();
	static void checkOTP(std::string code);
	static bool isLoggedIn();
};
