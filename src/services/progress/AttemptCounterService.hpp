#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class AttemptCounterService {
private:
	size_t cnt = 0;
public:
	void add();
	void submit();
};
