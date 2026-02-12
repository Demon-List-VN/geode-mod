#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

class AttemptCounter {
private:
	size_t cnt;
	geode::async::TaskHolder<geode::utils::web::WebResponse> holder;
public:
	void add();
	void submit();
};