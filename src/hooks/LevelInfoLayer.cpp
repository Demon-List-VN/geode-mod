#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/LevelInfoLayer.hpp> // DO NOT REMOVE
#include "../services/auth/AuthService.hpp"
#include "../services/level/LevelService.hpp"
#include "../utils/LevelInfoLayerUtils.hpp"

using namespace geode::prelude;
using namespace gdvn::level_info;

class $modify(LevelInfoLayer) {
	struct Fields {
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

			bool isLoggedIn = AuthService::isLoggedIn();

			auto layer = this;
			layer->retain();

			    LevelService::getLevel(id, [id, isLoggedIn, layer, level, loadingLabel](
					LevelInfoResponseDto const& model,
					web::WebResponse& res
				) {
					auto cleanup = [layer, loadingLabel]() {
						if (loadingLabel) {
							loadingLabel->removeFromParent();
						}

						layer->release();
				};

				if (!res.ok()) {
					cleanup();
					return;
				}

				if (!model.valid) {
					cleanup();
					log::warn("Failed to load GDVN level info for level {}", id);
					return;
				}

				std::vector<std::string> labels = getListInfoLabels(model.lists, isLoggedIn);

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
