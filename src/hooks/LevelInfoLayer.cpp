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

			    try {
					if (loadingLabel) {
						loadingLabel->removeFromParent();
						loadingLabel = nullptr;
					}

				    if (!res.ok()) {
						cleanup();
					    return;
				    }

				    auto resJson = res.json().unwrap();
			        std::vector<std::string> labels = getListInfoLabels(resJson, isLoggedIn);

			        if (!labels.empty()) {
					    auto btn = ButtonCreator().create(labels, level, layer);

					    layer->addChild(btn);
                    }

					cleanup();
			    } catch(...) {
					cleanup();
                    log::warn("Failed to load GDVN level info for level {}", id);
			    }
		    });
	    }

		return true;
	}
};
