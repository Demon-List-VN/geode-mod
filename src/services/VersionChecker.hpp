#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class VersionChecker {
	static async::TaskHolder<web::WebResponse> m_holder;
public:
	static void checkForUpdate();
};
