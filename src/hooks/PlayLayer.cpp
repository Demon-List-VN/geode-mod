#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/PlayLayer.hpp> // DO NOT REMOVE
#include <chrono>
#include "../services/AttemptCounter.hpp"
#include "../services/DeathCounter.hpp"
#include "../services/EventSubmitter.hpp"
#include "../services/RaidSubmitter.hpp"

using namespace geode::prelude;

class $modify(DTPlayLayer, PlayLayer) {
	struct Fields {
		bool hasRespawned = false;
		bool canSubmitRun = true;
		bool hasAttemptClock = false;
		double timestampAtAttemptStart = 0.0;
		std::chrono::steady_clock::time_point attemptStartedAt;
		AttemptCounter attemptCounter;
		DeathCounter deathCounter;
		EventSubmitter* eventSubmitter = nullptr;
		RaidSubmitter* raidSubmitter = nullptr;
	};

	bool isZeroStartRun() {
		return m_startPosObject == nullptr && !m_loadingStartPosition && !m_isPracticeMode && !m_isTestMode;
	}

	void invalidateRun(char const* step, char const* reason) {
		if (m_fields->canSubmitRun) {
			log::debug("[GDVN] PlayLayer::{} invalidating submission levelID={} reason={}", step, m_level ? m_level->m_levelID.value() : 0, reason);
		}
		m_fields->canSubmitRun = false;
	}

	bool validateRun(char const* step) {
		bool hasStartPos = m_startPosObject != nullptr;
		bool zeroStart = isZeroStartRun();

		log::debug(
			"[GDVN] PlayLayer::{} validate levelID={} zeroStart={} canSubmitRun={} startPos={} loadingStartPosition={} practice={} test={} started={} timestamp={} timePlayed={} hasRespawned={}",
			step,
			m_level ? m_level->m_levelID.value() : 0,
			zeroStart,
			m_fields->canSubmitRun,
			hasStartPos,
			m_loadingStartPosition,
			m_isPracticeMode,
			m_isTestMode,
			m_started,
			m_timestamp,
			m_timePlayed,
			m_fields->hasRespawned
		);

		if (hasStartPos) {
			invalidateRun(step, "startpos-enabled");
		}
		if (m_loadingStartPosition) {
			invalidateRun(step, "loading-start-position");
		}
		if (m_isPracticeMode) {
			invalidateRun(step, "practice-mode");
		}
		if (m_isTestMode) {
			invalidateRun(step, "test-mode");
		}

		return m_fields->canSubmitRun;
	}

	void resetAttemptClock(char const* step) {
		m_fields->hasAttemptClock = true;
		m_fields->attemptStartedAt = std::chrono::steady_clock::now();
		m_fields->timestampAtAttemptStart = m_timestamp;

		log::debug(
			"[GDVN] PlayLayer::{} resetAttemptClock levelID={} timestampAtStart={} timePlayed={} canSubmitRun={}",
			step,
			m_level ? m_level->m_levelID.value() : 0,
			m_fields->timestampAtAttemptStart,
			m_timePlayed,
			m_fields->canSubmitRun
		);
	}

	bool passesTimestampCheck(char const* step) {
		if (!m_fields->canSubmitRun) {
			log::debug("[GDVN] PlayLayer::{} timestampCheck skipped levelID={} reason=run-invalid", step, m_level ? m_level->m_levelID.value() : 0);
			return false;
		}

		if (!m_fields->hasAttemptClock) {
			log::debug("[GDVN] PlayLayer::{} timestampCheck skipped levelID={} reason=no-attempt-clock", step, m_level ? m_level->m_levelID.value() : 0);
			return false;
		}

		auto const wallElapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_fields->attemptStartedAt).count();
		if (wallElapsed < 0.25) {
			log::debug("[GDVN] PlayLayer::{} timestampCheck skipped levelID={} reason=short-interval wallElapsed={}", step, m_level ? m_level->m_levelID.value() : 0, wallElapsed);
			return true;
		}

		auto const timestampElapsed = std::max(0.0, m_timestamp - m_fields->timestampAtAttemptStart);
		auto const allowedElapsed = wallElapsed * 1.01;
		bool const valid = timestampElapsed <= allowedElapsed;

		log::debug(
			"[GDVN] PlayLayer::{} timestampCheck levelID={} timestampElapsed={} wallElapsed={} allowedElapsed={} valid={}",
			step,
			m_level ? m_level->m_levelID.value() : 0,
			timestampElapsed,
			wallElapsed,
			allowedElapsed,
			valid
		);

		if (!valid) {
			invalidateRun(step, "timestamp-check-failed");
		}

		return valid;
	}

	bool canRecordOrSubmit(char const* step) {
		if (!validateRun(step)) {
			log::debug("[GDVN] PlayLayer::{} skipped levelID={} reason=invalid-run", step, m_level ? m_level->m_levelID.value() : 0);
			return false;
		}

		if (!passesTimestampCheck(step)) {
			log::debug("[GDVN] PlayLayer::{} skipped levelID={} reason=timestamp-check", step, m_level ? m_level->m_levelID.value() : 0);
			return false;
		}

		return true;
	}

	bool init(GJGameLevel * level, bool p1, bool p2) {
		if (!PlayLayer::init(level, p1, p2)) {
			return false;
		}

		int id = level->m_levelID.value();
		auto best = level->m_normalPercent.value();

		m_fields->deathCounter = DeathCounter(id, best >= 100);
		m_fields->eventSubmitter = new EventSubmitter(id);
		m_fields->raidSubmitter = new RaidSubmitter(id);

		log::debug(
			"[GDVN] PlayLayer::init levelID={} startPos={} loadingStartPosition={} practice={} test={}",
			id,
			m_startPosObject != nullptr,
			m_loadingStartPosition,
			m_isPracticeMode,
			m_isTestMode
		);

		return true;
	}

	void startGame() {
		PlayLayer::startGame();

		validateRun("startGame");
		resetAttemptClock("startGame");
	}

	void destroyPlayer(PlayerObject * player, GameObject * p1) {
		PlayLayer::destroyPlayer(player, p1);

		if (!player->m_isDead) {
			return;
		}

		if (!canRecordOrSubmit("destroyPlayer")) {
			return;
		}

		m_fields->attemptCounter.add();

		if (!m_level->isPlatformer() && !m_isPracticeMode) {
			m_fields->deathCounter.add(this->getCurrentPercentInt());
			m_fields->eventSubmitter->record(this->getCurrentPercent());
			m_fields->raidSubmitter->record(this->getCurrentPercent());
		}
	}

	void levelComplete() {
		PlayLayer::levelComplete();

		if (!m_isPracticeMode && canRecordOrSubmit("levelComplete")) {
			m_fields->eventSubmitter->record(100);
			m_fields->raidSubmitter->record(100);
			m_fields->deathCounter.setCompleted(true);
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		m_fields->hasRespawned = true;
		validateRun("resetLevel");
		log::debug("[GDVN] PlayLayer::resetLevel levelID={} canSubmitRun={}", m_level ? m_level->m_levelID.value() : 0, m_fields->canSubmitRun);
	}

	void onQuit() {
		bool const shouldSubmit = canRecordOrSubmit("onQuit");
		log::debug("[GDVN] PlayLayer::onQuit submitCheck levelID={} shouldSubmit={}", m_level ? m_level->m_levelID.value() : 0, shouldSubmit);

		PlayLayer::onQuit();

		if (shouldSubmit) {
			m_fields->attemptCounter.submit();
			m_fields->deathCounter.submit();
		} else {
			log::debug("[GDVN] PlayLayer::onQuit skipped submissions levelID={} reason=validation-failed", m_level ? m_level->m_levelID.value() : 0);
		}

		delete m_fields->eventSubmitter;
		delete m_fields->raidSubmitter;
	}
};
