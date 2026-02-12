#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "../lib/VersionChecker.hpp"

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
		}

		return true;
	}
};
