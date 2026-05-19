#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <atomic>
#include <memory>

using namespace geode::prelude;

class PvpSubmitter {
	struct State {
		int levelID = 0;
		int matchID = 0;
		float best = 0;
		bool platformer = false;
		std::atomic<bool> inPvp{ false };

		explicit State(int levelID = 0) : levelID(levelID) {}
	};

	std::shared_ptr<State> m_state;
	static async::TaskHolder<web::WebResponse> m_get_holder, m_put_holder;

	void submit(bool completed = false);

public:
	PvpSubmitter();
	~PvpSubmitter();
	PvpSubmitter(int levelID);
	bool isPlatformerPvp() const;
	void record(float progress);
	void recordCheckpoint(int count);
	void completePlatformer(int count);
};
