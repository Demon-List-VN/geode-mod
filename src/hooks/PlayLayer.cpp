#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/PlayLayer.hpp> // DO NOT REMOVE
#include <Geode/binding/CheckpointGameObject.hpp>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include "../services/progress/AttemptCounterService.hpp"
#include "../services/progress/DeathCounterService.hpp"
#include "../services/event/EventSubmitterService.hpp"
#include "../services/event/RaidSubmitterService.hpp"
#include "../services/pvp/PvpSubmitterService.hpp"
#include "../services/pvp/PvpOverlayService.hpp"
#include "../services/cheat/CheatGuardService.hpp"
#include "../services/auth/AuthService.hpp"

using namespace geode::prelude;

class $modify(DTPlayLayer, PlayLayer) {
	struct Fields {
		bool hasRespawned = false;
		bool isCheatedRun = false;
		std::string cheatReason;
		AttemptCounterService attemptCounter;
		DeathCounterService deathCounter;
		std::unique_ptr<EventSubmitterService> eventSubmitter;
		std::unique_ptr<RaidSubmitterService> raidSubmitter;
		std::unique_ptr<PvpSubmitterService> pvpSubmitter;
		std::unique_ptr<PvpOverlayService> pvpOverlay;
		std::unordered_set<int> platformerCheckpointIds;
		int platformerCheckpointCount = 0;
		bool lastPracticeMode = false;
	};

	static void onModify(auto& self) {
		(void)self.setHookPriorityPre("PlayLayer::destroyPlayer", Priority::First);
	}

	bool isDamageBypassActive() {
		return m_isIgnoreDamageEnabled || m_ignoreDamage;
	}

	void submitPvpPlayModeIfChanged(bool force = false) {
		const bool isPractice = m_isPracticeMode;

		if (!force && m_fields->lastPracticeMode == isPractice) {
			return;
		}

		m_fields->lastPracticeMode = isPractice;

		if (m_fields->pvpSubmitter) {
			m_fields->pvpSubmitter->submitPlayMode(isPractice ? "practice" : "normal");
		}
	}

	void markRunCheated(std::string const& reason) {
		if (m_fields->isCheatedRun) {
			return;
		}

		m_fields->isCheatedRun = true;
		m_fields->cheatReason = reason;
		log::warn("Run marked as cheated on level {}: {}", m_level->m_levelID.value(), reason);
	}

	void refreshCheatGuardReason() {
		if (m_fields->isCheatedRun) {
			return;
		}

		if (auto reason = CheatGuardService::getGameplayCheatReason()) {
			markRunCheated(std::string(*reason));
		}
	}

	bool isRunCheated() {
		refreshCheatGuardReason();

		if (!m_fields->isCheatedRun && isDamageBypassActive()) {
			markRunCheated("damage bypass");
		}

		return m_fields->isCheatedRun;
	}

	void checkNoclip(PlayerObject* player) {
		if (!m_fields->isCheatedRun && !player->m_isDead && !m_levelEndAnimationStarted) {
			markRunCheated("noclip");
		}
	}

	bool init(GJGameLevel * level, bool p1, bool p2) {
		if (!PlayLayer::init(level, p1, p2)) {
			return false;
		}

		int id = level->m_levelID.value();
		auto best = level->m_normalPercent.value();

		m_fields->deathCounter = DeathCounterService(id, best >= 100);
		m_fields->eventSubmitter = std::make_unique<EventSubmitterService>(id);
		m_fields->raidSubmitter = std::make_unique<RaidSubmitterService>(id);
		m_fields->lastPracticeMode = m_isPracticeMode;
		m_fields->pvpSubmitter = std::make_unique<PvpSubmitterService>(
			id,
			m_isPracticeMode ? "practice" : "normal"
		);
		refreshCheatGuardReason();

		if (AuthService::isLoggedIn() && !m_isPracticeMode) {
			m_fields->pvpOverlay = std::make_unique<PvpOverlayService>(this, id, m_fields->pvpSubmitter.get());
		}

		return true;
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);

		if (!m_level->isPlatformer() && !m_isPracticeMode) {
			isRunCheated();
		}

		if (m_fields->pvpOverlay) {
			m_fields->pvpOverlay->update(dt);
		}

		submitPvpPlayModeIfChanged();
	}

	void togglePracticeMode(bool practiceMode) {
		PlayLayer::togglePracticeMode(practiceMode);
		submitPvpPlayModeIfChanged(true);
	}

	void destroyPlayer(PlayerObject * player, GameObject * p1) {
		PlayLayer::destroyPlayer(player, p1);

		if (m_level->isPlatformer() || m_isPracticeMode) {
			return;
		}

		checkNoclip(player);

		if (!player->m_isDead) {
			return;
		}

		bool isCheated = isRunCheated();
		log::info("Run ended on level {} at {}%: {}", m_level->m_levelID.value(), this->getCurrentPercentInt(), isCheated ? "cheated" : "not cheated");

		if (isCheated) {
			return;
		}

		const float progress = std::min(this->getCurrentPercent(), 99.99f);

		m_fields->attemptCounter.add();
		m_fields->deathCounter.add(progress);
		m_fields->eventSubmitter->record(progress);
		m_fields->raidSubmitter->record(progress);
		m_fields->pvpSubmitter->recordDeath(progress);
	}

	void levelComplete() {
		PlayLayer::levelComplete();

		if (!m_isPracticeMode) {
			if (m_level->isPlatformer()) {
				m_fields->pvpSubmitter->completePlatformer(m_fields->platformerCheckpointCount);
				return;
			}

			bool isCheated = isRunCheated();
			log::info("Run completed on level {}: {}", m_level->m_levelID.value(), isCheated ? "cheated" : "not cheated");

			if (isCheated) {
				return;
			}

			m_fields->eventSubmitter->record(100);
			m_fields->raidSubmitter->record(100);
			m_fields->pvpSubmitter->record(100);
			m_fields->pvpSubmitter->flushDeathCount();
			m_fields->deathCounter.setCompleted(true);
		}
	}

	void checkpointActivated(CheckpointGameObject* object) {
		PlayLayer::checkpointActivated(object);

		if (!object || !m_level->isPlatformer() || m_isPracticeMode) {
			return;
		}

		const int checkpointID = object->m_uniqueID > 0 ? object->m_uniqueID : object->m_objectID;
		if (checkpointID <= 0) {
			return;
		}

		if (m_fields->platformerCheckpointIds.insert(checkpointID).second) {
			m_fields->platformerCheckpointCount = static_cast<int>(m_fields->platformerCheckpointIds.size());
			m_fields->pvpSubmitter->recordCheckpoint(m_fields->platformerCheckpointCount);
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		if (!m_level->isPlatformer()) {
			m_fields->platformerCheckpointIds.clear();
			m_fields->platformerCheckpointCount = 0;
		}
		m_fields->hasRespawned = true;
		m_fields->isCheatedRun = false;
		m_fields->cheatReason.clear();
		refreshCheatGuardReason();
	}

	void onQuit() {
		m_fields->pvpOverlay.reset();
		bool isCheated = isRunCheated();

		if (isCheated) {
			log::info("Skipping gameplay API submissions because the run is cheated");
		} else if (!m_level->isPlatformer()) {
			m_fields->attemptCounter.submit();
			m_fields->deathCounter.submit();
		}
		if (!m_level->isPlatformer()) {
			m_fields->pvpSubmitter->flushDeathCount();
		}

		m_fields->eventSubmitter.reset();
		m_fields->raidSubmitter.reset();
		m_fields->pvpSubmitter.reset();

		PlayLayer::onQuit();
	}
};
