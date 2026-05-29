#pragma once

#include "../utils/DeathData.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class DeathCounterService {
private:
	DeathData deathData;
    bool completed = false;
public:
	DeathCounterService();
	DeathCounterService(int id, bool completed);
	void add(int percent);
	void submit();
    void setCompleted(bool completed) {
        this->completed = completed;
    }
};
