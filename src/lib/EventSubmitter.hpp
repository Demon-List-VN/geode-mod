#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <atomic>

using namespace geode::prelude;

class EventSubmitter {
	int levelID = 0;
	float best = 0;
	std::atomic<bool> inEvent{ false };
	async::TaskHolder<web::WebResponse> getHolder, putHolder;

	void submit();
public:
	EventSubmitter();
	~EventSubmitter();
	EventSubmitter(int levelID);
	void record(float progress);
};