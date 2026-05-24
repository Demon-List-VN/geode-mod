#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class VersionChecker {
	static async::TaskHolder<web::WebResponse> m_holder;
	static void downloadUpdate();
public:
	static void checkForUpdate(bool notifyIfCurrent = false);
};
