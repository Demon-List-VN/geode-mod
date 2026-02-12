#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/LevelInfoLayer.hpp> // DO NOT REMOVE
#include "../common.hpp"

using namespace geode::prelude;

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
