#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <string>

using namespace geode::prelude;

class PvpSubmitter {
	struct State {
		int levelID = 0;
		int matchID = 0;
		std::string currentUid;
		float sessionBest = 0;
		float apiBest = 0;
		float submittedBest = 0;
		float progressRetryDelay = 0.0f;
		float progressRetryValue = 0.0f;
		std::array<size_t, 100> pendingDeathCount = {};
		std::atomic<bool> deathSubmitInFlight{ false };
		std::atomic<int> progressSubmitGeneration{ 0 };
		int progressRetryGeneration = 0;
		bool progressRetryPending = false;
		bool progressRetryCompleted = false;
		bool platformer = false;
		std::atomic<bool> inPvp{ false };

		explicit State(int levelID = 0) : levelID(levelID) {}
	};

	std::shared_ptr<State> m_state;
	static async::TaskHolder<web::WebResponse> m_get_holder, m_put_holder, m_death_holder;

	void submit(bool completed = false);
	static void submitProgress(
		std::shared_ptr<State> state,
		float progress,
		bool completed,
		int generation
	);
	static void scheduleProgressRetry(
		std::weak_ptr<State> state,
		float progress,
		bool completed,
		int generation
	);
	static void submitDeathCount(std::shared_ptr<State> state);
	static std::string serializeDeathCount(std::array<size_t, 100> const& count);
	static size_t sumDeathCount(std::array<size_t, 100> const& count);

public:
	PvpSubmitter();
	~PvpSubmitter();
	PvpSubmitter(int levelID);
	void update(float dt);
	bool isPlatformerPvp() const;
	void syncApiProgress(float progress);
	void record(float progress);
	void recordDeath(float progress);
	void flushDeathCount();
	void recordCheckpoint(int count);
	void completePlatformer(int count);
};
