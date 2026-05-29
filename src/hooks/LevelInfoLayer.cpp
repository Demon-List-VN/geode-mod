#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/LevelInfoLayer.hpp> // DO NOT REMOVE
#include "../common.hpp"
#include "../services/AuthService.hpp"
#include "../utils/LevelInfoLayerUtils.hpp"

using namespace geode::prelude;
using namespace gdvn::level_info;

class $modify(LevelInfoLayer) {
	struct Fields {
		async::TaskHolder<web::WebResponse> m_holder;
		bool m_confirmedLoggedOutPlay = false;
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

			bool isLoggedIn = AuthService::isLoggedIn();

		    if (isLoggedIn) {
			    req.header("Authorization", "Bearer " + AuthService::getToken());
		    }

			auto layer = this;
			layer->retain();

		    m_fields->m_holder.spawn(req.get(API_URL + "/lists/levels/" + std::to_string(id) + "/starred"), [layer, level, loadingLabel, id, isLoggedIn](web::WebResponse res) mutable {
				auto cleanup = [&]() {
					if (loadingLabel) {
						loadingLabel->removeFromParent();
						loadingLabel = nullptr;
					}

					layer->release();
				};

				if (!res.ok()) {
					cleanup();
					return;
				}

				auto resJsonResult = res.json();
				if (!resJsonResult) {
					cleanup();
					log::warn("Failed to load GDVN level info for level {}", id);
					return;
				}

				auto resJson = resJsonResult.unwrap();
				std::vector<std::string> labels = getListInfoLabels(resJson, isLoggedIn);

				if (!labels.empty()) {
					auto btn = ButtonCreator().create(labels, level, layer);

					layer->addChild(btn);
				}

				cleanup();
		    });
	    }

		return true;
	}

	void onPlay(CCObject* sender) {
		if (!AuthService::isLoggedIn() && !m_fields->m_confirmedLoggedOutPlay) {
			this->retain();
			if (sender) {
				sender->retain();
			}

			geode::createQuickPopup(
				"GDVN",
				"You are not logged in, progress will not be saved to GDVN server.",
				"Cancel",
				"Play",
				[this, sender](auto, bool btn2) {
					if (btn2) {
						m_fields->m_confirmedLoggedOutPlay = true;
						LevelInfoLayer::onPlay(sender);
						m_fields->m_confirmedLoggedOutPlay = false;
					}

					if (sender) {
						sender->release();
					}
					this->release();
				}
			);
			return;
		}

		LevelInfoLayer::onPlay(sender);
	}
};
