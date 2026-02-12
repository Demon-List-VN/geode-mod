#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "../services/VersionChecker.hpp"
#include "../services/OTPLogin.hpp"

using namespace geode::prelude;

class $modify(MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		static bool hasChecked = false;

		if (!hasChecked) {
			hasChecked = true;
			VersionChecker::checkForUpdate();

			if (!OTPLogin::isLoggedIn()) {
				OTPLogin::showLoginPrompt();
			}
		}

		return true;
	}
};
