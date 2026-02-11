#pragma once

#include "../utils/DeathData.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

class DeathCounter {
private:
	DeathData deathData;
    bool completed = false;
public:
	DeathCounter();
	DeathCounter(int id, bool completed);
	void add(int percent);
	void submit(geode::async::TaskHolder<geode::utils::web::WebResponse>* holder);
    void setCompleted(bool completed) {
        this->completed = completed;
    }
};