#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "../services/VersionChecker.hpp"
#include "../services/AuthService.hpp"

using namespace geode::prelude;

class $modify(MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

	    bool loginShown = Mod::get()->getSavedValue<bool>("login-shown", false);

	    if (!loginShown && !AuthService::isLoggedIn()) {
	        Mod::get()->setSavedValue<bool>("login-shown", true);
            AuthService::login();
	    }

	    static bool hasChecked = false;

		if (!hasChecked) {
			hasChecked = true;
			VersionChecker::checkForUpdate();
		    AuthService::check();
		}

		return true;
	}
};
