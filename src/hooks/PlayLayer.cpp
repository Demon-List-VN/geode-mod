#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/PlayLayer.hpp> // DO NOT REMOVE
#include "../lib/AttemptCounter.hpp"
#include "../lib/DeathCounter.hpp"
#include "../lib/EventSubmitter.hpp"
#include "../lib/RaidSubmitter.hpp"

using namespace geode::prelude;

class $modify(DTPlayLayer, PlayLayer) {
	struct Fields {
		bool hasRespawned = false;
		AttemptCounter attemptCounter;
		DeathCounter deathCounter;
		EventSubmitter *eventSubmitter;
		RaidSubmitter *raidSubmitter;
	};

	bool init(GJGameLevel * level, bool p1, bool p2) {
		if (!PlayLayer::init(level, p1, p2)) {
			return false;
		}

		int id = level->m_levelID.value();
		auto best = level->m_normalPercent.value();

		m_fields->deathCounter = DeathCounter(id, best >= 100);
		m_fields->eventSubmitter = new EventSubmitter(id);
		m_fields->raidSubmitter = new RaidSubmitter(id);

		return true;
	}

	void destroyPlayer(PlayerObject * player, GameObject * p1) {
		PlayLayer::destroyPlayer(player, p1);

		if (!player->m_isDead) {
			return;
		}

		if (!m_fields->hasRespawned) {
			return;
		}

		m_fields->hasRespawned = false;
		m_fields->attemptCounter.add();

		if (!m_level->isPlatformer() && !m_isPracticeMode) {
			m_fields->deathCounter.add(this->getCurrentPercentInt());
			m_fields->eventSubmitter->record(this->getCurrentPercent());
			m_fields->raidSubmitter->record(this->getCurrentPercent());
		}
	}

	void levelComplete() {
		PlayLayer::levelComplete();

		if (!m_isPracticeMode) {
			m_fields->eventSubmitter->record(100);
			m_fields->raidSubmitter->record(100);
		    m_fields->deathCounter.setCompleted(true);
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		m_fields->hasRespawned = true;
	}

	void onQuit() {
		PlayLayer::onQuit();

		m_fields->attemptCounter.submit();
		m_fields->deathCounter.submit();

		delete m_fields->eventSubmitter;
		delete m_fields->raidSubmitter;
	}
};
