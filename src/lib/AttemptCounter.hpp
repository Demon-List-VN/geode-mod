#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

class AttemptCounter {
private:
	size_t cnt;
public:
	void add();
	void submit(geode::async::TaskHolder<geode::utils::web::WebResponse>* holder);
};