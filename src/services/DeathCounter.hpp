#pragma once

#include "../utils/DeathData.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class DeathCounter {
private:
	DeathData deathData;
    bool completed = false;
	static geode::async::TaskHolder<geode::utils::web::WebResponse> m_holder;
public:
	DeathCounter();
	DeathCounter(int id, bool completed);
	void add(int percent);
	void submit();
    void setCompleted(bool completed) {
        this->completed = completed;
    }
};