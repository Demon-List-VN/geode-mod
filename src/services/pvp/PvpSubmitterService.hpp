#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <string>

using namespace geode::prelude;

class PvpSubmitterService {
	struct State {
		int levelID = 0;
		int matchID = 0;
		float best = 0;
		std::array<size_t, 100> pendingDeathCount = {};
		std::atomic<bool> deathSubmitInFlight{ false };
		bool platformer = false;
		std::string playMode = "normal";
		std::string submittedPlayMode;
		std::atomic<bool> inPvp{ false };

		explicit State(int levelID = 0) : levelID(levelID) {}
	};

	std::shared_ptr<State> m_state;

	void submit(bool completed = false);
	static void submitDeathCount(std::shared_ptr<State> state);
	static void submitPlayMode(std::shared_ptr<State> state, std::string const& playMode);
	static std::string serializeDeathCount(std::array<size_t, 100> const& count);
	static size_t sumDeathCount(std::array<size_t, 100> const& count);

public:
	PvpSubmitterService();
	~PvpSubmitterService();
	PvpSubmitterService(int levelID, std::string playMode = "normal");
	bool isPlatformerPvp() const;
	void submitPlayMode(std::string const& playMode);
	void record(float progress);
	void recordDeath(float progress);
	void flushDeathCount();
	void recordCheckpoint(int count);
	void completePlatformer(int count);
};
