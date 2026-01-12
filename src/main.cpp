#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/PlayLayer.hpp> // DO NOT REMOVE
#include <Geode/modify/LevelInfoLayer.hpp> // DO NOT REMOVE
#include "lib/AttemptCounter.hpp"
#include "lib/DeathCounter.hpp"
#include "lib/EventSubmitter.hpp"
#include "lib/RaidSubmitter.hpp"
#include "common.hpp"

using namespace geode::prelude;

EventListener<web::WebTask> attemptCounterListener, deathCounterListener;

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
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		m_fields->hasRespawned = true;
	}

	void onQuit() {
		PlayLayer::onQuit();

		m_fields->attemptCounter.submit(&(attemptCounterListener));
		m_fields->deathCounter.submit(&(deathCounterListener));

		delete m_fields->eventSubmitter;
		delete m_fields->raidSubmitter;
	}
};

CCLabelBMFont* createLabel(GJGameLevel* level, std::string str, int order) {
	int offset = (level->m_coins == 0) ? 17 : 4;
	auto size = CCDirector::sharedDirector()->getWinSize();
	int yoffset = 2;

	CCLabelBMFont* label = CCLabelBMFont::create(str.c_str(), "goldFont.fnt");

	label->setPosition({ size.width / 2 - 100, size.height / 2 + offset + yoffset + order * -10 });
	label->setScale(0.3f);

	return label;
}

class ButtonCreator {
public:
	void onButton(CCObject* sender) {
		int id = sender->getTag();
		std::string url = "https://demonlistvn.com/levels/" + std::to_string(id);

		web::openLinkInBrowser(url);
	}

	CCMenu* create(std::vector<std::string> labels, GJGameLevel* level, CCLayer* layer) {
		int offset = (level->m_coins == 0) ? 17 : 4;
		auto size = CCDirector::sharedDirector()->getWinSize();

		std::string text;

		for (std::string& s : labels) {
			text += s;
			text += '\n';
		}

		text.pop_back();

		CCLabelBMFont* label = CCLabelBMFont::create(text.c_str(), "goldFont.fnt");

		label->setScale(0.3f);
		label->setAlignment(kCCTextAlignmentCenter);

		auto btn = CCMenuItemSpriteExtra::create(
			label, layer, menu_selector(ButtonCreator::onButton)
		);

		btn->setTag(level->m_levelID.value());

		auto menu = CCMenu::create();

		menu->addChild(btn);
		menu->setPosition({ size.width / 2 - 100, size.height / 2 + offset });

		return menu;
	}
};

class $modify(LevelInfoLayer) {
	struct Fields {
		EventListener<web::WebTask> m_listener;
	};

	bool init(GJGameLevel* level, bool a) {
		if (!LevelInfoLayer::init(level, a)) {
			return false;
		}

		if (level->m_demonDifficulty != 6 && level->m_demonDifficulty != 5) {
			return true;
		}

		int id = level->m_levelID.value();
		auto loadingLabel = createLabel(level, "...", 0);

		this->addChild(loadingLabel);

		m_fields->m_listener.bind([this, level, loadingLabel](web::WebTask::Event* e) {
			try {
				if (this == nullptr) {
					return;
				}

				if (web::WebResponse* res = e->getValue()) {
					loadingLabel->removeFromParent();

					if (!res->ok()) {
						return;
					}

					auto resJson = res->json().unwrap();

					if (resJson["rating"].isNumber() && resJson["flPt"].isNumber()) {
						auto tmp = resJson["rating"].asInt();
						std::string dl = "DL: " + std::to_string(resJson["rating"].asInt().unwrap()) + " (#" + std::to_string(resJson["dlTop"].asInt().unwrap()) + ")";
						std::string fl = "FL: " + std::to_string(resJson["flPt"].asInt().unwrap()) + " (#" + std::to_string(resJson["flTop"].asInt().unwrap()) + ")";

						auto btn = ButtonCreator().create({ dl, fl }, level, this);

						this->addChild(btn);
					}
					else if (resJson["rating"].isNumber()) {
						std::string dl = "DL: " + std::to_string(resJson["rating"].asInt().unwrap()) + " (#" + std::to_string(resJson["dlTop"].asInt().unwrap()) + ")";
						auto btn = ButtonCreator().create({ dl }, level, this);

						this->addChild(btn);
					}
					else if (resJson["flPt"].isNumber()) {
						std::string fl = "FL: " + std::to_string(resJson["flPt"].asDouble().unwrap()) + " (#" + std::to_string(resJson["flTop"].asInt().unwrap()) + ")";
						auto btn = ButtonCreator().create({ fl }, level, this);

						this->addChild(btn);
					}
				}
				else if (e->isCancelled()) {
					loadingLabel->removeFromParent();
				}
			} catch(...) {
				loadingLabel->removeFromParent();
			}
		});

		web::WebRequest req = web::WebRequest();
		m_fields->m_listener.setFilter(req.get(API_URL + "/levels/" + std::to_string(id)));

		return true;
	}
};

// Helper function to open a level's info layer
void openLevelInfoLayer(GJGameLevel* level) {
	auto infoLayer = LevelInfoLayer::create(level, false);
	auto scene = CCDirector::sharedDirector()->getRunningScene();
	if (scene && infoLayer) {
		scene->addChild(infoLayer);
	}
}

$on_mod(Loaded) {
	Loader::get()->queueInMainThread([] {
		// Register URI handler for opening levels
		// URI format: demonlistvn://level/123
		geode::Mod::get()->registerLinkCallback("level", [](std::string const& data) -> Task<Result<>> {
			int levelID;
			try {
				// Parse the level ID from the URI data
				levelID = std::stoi(data);
			} catch (std::exception const& e) {
				co_return Err("Failed to parse level ID: {}", e.what());
			}
			
			// Get the game level manager
			auto glm = GameLevelManager::sharedState();
			
			// Try to get the level from stored levels first
			auto level = glm->m_onlinelevels->objectForKey(std::to_string(levelID));
			
			if (level) {
				// If we have the level cached, open it directly
				auto gjLevel = static_cast<GJGameLevel*>(level);
				openLevelInfoLayer(gjLevel);
			} else {
				// If not cached, we need to fetch it from the server
				auto searchObj = GJSearchObject::create(SearchType::Search, std::to_string(levelID));
				glm->getOnlineLevels(searchObj);
				
				// Schedule a callback to check when the level is loaded
				// Note: This uses a fixed delay which may fail if server is slow
				Loader::get()->queueInMainThread([levelID] {
					auto glm = GameLevelManager::sharedState();
					auto level = glm->m_onlinelevels->objectForKey(std::to_string(levelID));
					if (level) {
						auto gjLevel = static_cast<GJGameLevel*>(level);
						openLevelInfoLayer(gjLevel);
					}
					// If level is still not loaded, it will silently fail
				}, 1.5f);
			}
			
			co_return Ok();
		});
	});
}