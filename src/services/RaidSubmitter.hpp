#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <atomic>
#include <memory>

using namespace geode::prelude;

class RaidSubmitter {
	struct State {
		int levelID = 0;
		float best = 0;
		std::atomic<bool> inEvent{ false };

		explicit State(int levelID = 0) : levelID(levelID) {}
	};

	std::shared_ptr<State> m_state;
	static async::TaskHolder<web::WebResponse> m_get_holder, m_put_holder;

	void submit();
public:
	RaidSubmitter();
	~RaidSubmitter();
	RaidSubmitter(int levelID);
	void record(float progress);
};
