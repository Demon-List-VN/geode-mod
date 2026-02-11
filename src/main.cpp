#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/PlayLayer.hpp> // DO NOT REMOVE
#include <Geode/modify/LevelInfoLayer.hpp> // DO NOT REMOVE
#include <Geode/modify/MenuLayer.hpp>
#include "lib/AttemptCounter.hpp"
#include "lib/DeathCounter.hpp"
#include "lib/EventSubmitter.hpp"
#include "lib/RaidSubmitter.hpp"
#include "lib/VersionChecker.hpp"
#include "common.hpp"

using namespace geode::prelude;

async::TaskHolder<web::WebResponse> attemptCounterHolder, deathCounterHolder;

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

		m_fields->attemptCounter.submit(&(attemptCounterHolder));
		m_fields->deathCounter.submit(&(deathCounterHolder));

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
		async::TaskHolder<web::WebResponse> m_holder;
	};

	bool init(GJGameLevel* level, bool a) {
		if (!LevelInfoLayer::init(level, a)) {
			return false;
		}

		int id = level->m_levelID.value();
	    auto showLevelInfo = Mod::get()->getSettingValue<bool>("show-level-info");

	    if (showLevelInfo) {
		    auto loadingLabel = createLabel(level, "...", 0);

		    this->addChild(loadingLabel);

		    web::WebRequest req = web::WebRequest();
		    m_fields->m_holder.spawn(req.get(API_URL + "/levels/" + std::to_string(id)), [this, level, loadingLabel](web::WebResponse res) {
			    try {
				    loadingLabel->removeFromParent();

				    if (!res.ok()) {
					    return;
				    }

				    auto resJson = res.json().unwrap();
			        bool isPlatformer = false, isChallenge = false;

			        if (resJson["isPlatformer"].isBool()) {
                        isPlatformer = resJson["isPlatformer"].asBool().unwrap();
                    }

			        if (resJson["isChallenge"].isBool()) {
                        isChallenge = resJson["isChallenge"].asBool().unwrap();
                    }

			        std::string list;

			        if (!isPlatformer && !isChallenge) {
                        list = "DL: ";
                    }
                    else if (isPlatformer) {
                        list = "PL: ";
                    }
                    else if (isChallenge) {
                        list = "CL: ";
                    }

				    if (resJson["rating"].isNumber() && resJson["flPt"].isNumber()) {
					    std::string dl = list + std::to_string(resJson["rating"].asInt().unwrap());

				        if (!isPlatformer && !isChallenge) {
                            dl += " (#" + std::to_string(resJson["dlTop"].asInt().unwrap()) + ")";
                        }
                        else if (isPlatformer) {
                            dl += " (#" + std::to_string(resJson["plTop"].asInt().unwrap()) + ")";
                        }

					    std::string fl = "FL: " + std::to_string(resJson["flPt"].asInt().unwrap()) + " (#" + std::to_string(resJson["flTop"].asInt().unwrap()) + ")";

					    auto btn = ButtonCreator().create({ dl, fl }, level, this);

					    this->addChild(btn);
				    }
				    else if (resJson["rating"].isNumber()) {
				        std::string dl = list + std::to_string(resJson["rating"].asInt().unwrap());

                        if (!isPlatformer && !isChallenge) {
                            dl += " (#" + std::to_string(resJson["dlTop"].asInt().unwrap()) + ")";
                        }
                        else if (isPlatformer) {
                            dl += " (#" + std::to_string(resJson["plTop"].asInt().unwrap()) + ")";
                        }

				        auto btn = ButtonCreator().create({ dl }, level, this);

					    this->addChild(btn);
				    }
				    else if (resJson["flPt"].isNumber()) {
					    std::string fl = "FL: " + std::to_string(resJson["flPt"].asDouble().unwrap()) + " (#" + std::to_string(resJson["flTop"].asInt().unwrap()) + ")";
					    auto btn = ButtonCreator().create({ fl }, level, this);

					    this->addChild(btn);
				    }
			    } catch(...) {
				    loadingLabel->removeFromParent();
			    }
		    });
	    }

		return true;
	}
};

class $modify(MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		static bool hasChecked = false;

		if (!hasChecked) {
			hasChecked = true;
			VersionChecker::checkForUpdate();
		}

		return true;
	}
};