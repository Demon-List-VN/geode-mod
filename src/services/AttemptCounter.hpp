#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class AttemptCounter {
private:
	size_t cnt;
	static geode::async::TaskHolder<geode::utils::web::WebResponse> m_holder;
public:
	void add();
	void submit();
};