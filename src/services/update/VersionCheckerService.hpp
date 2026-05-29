#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class VersionCheckerService {
	static void downloadUpdate();
public:
	static void checkForUpdate(bool notifyIfCurrent = false);
};
