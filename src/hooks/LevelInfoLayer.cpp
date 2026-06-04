#include "../dtos/level/LevelListDto.hpp"
#include "../factories/LevelInfoLayerFactory.hpp"
#include "../services/auth/AuthService.hpp"
#include "../services/level/LevelService.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp> // DO NOT REMOVE
#include <Geode/utils/web.hpp>

#include <memory>

using namespace geode::prelude;

namespace {

std::string formatNumber(double number) {
    int integer = static_cast<int>(number);

    if (number == static_cast<double>(integer)) {
        return std::to_string(integer);
    }

    std::string formatted = fmt::format("{:.1f}", number);

    while (formatted.size() > 1 && formatted.ends_with('0')) {
        formatted.pop_back();
    }

    if (formatted.ends_with('.')) {
        formatted.pop_back();
    }

    return formatted;
}

std::string getListValue(LevelListDto const& list) {
    if (!list.item.position && !list.item.rating) {
        return "";
    }

    std::vector<std::string> values;

    if (list.isTopMode() && list.item.position) {
        values.push_back("#" + formatNumber(*list.item.position));
    }

    if (list.item.rating) {
        values.push_back(formatNumber(*list.item.rating) + "pt");
    }

    if (!list.isTopMode() && values.empty() && list.item.position) {
        values.push_back("#" + formatNumber(*list.item.position));
    }

    std::string text;

    for (auto& value : values) {
        if (!text.empty()) {
            text += " / ";
        }

        text += value;
    }

    return text;
}

std::vector<std::string> getListInfoLabels(std::vector<LevelListDto> const& lists, bool isLoggedIn) {
    std::vector<std::string> officialLabels;
    std::vector<std::string> starredLabels;

    for (auto const& list : lists) {
        std::string value = getListValue(list);

        if (value.empty()) {
            continue;
        }

        auto label = list.label() + ": " + value;

        if (list.isStarredList()) {
            starredLabels.push_back(label);
        }

        if (list.isOfficial) {
            officialLabels.push_back(label);
        }
    }

    auto labels = (isLoggedIn && !starredLabels.empty()) ? starredLabels : officialLabels;
    bool hasMore = labels.size() > 5;

    if (hasMore) {
        labels.resize(5);
        labels.push_back("...");
    }

    return labels;
}

} // namespace

class $modify(GDVNLevelInfoLayer, LevelInfoLayer) {
    struct LevelInfoRequestState {
        bool active = true;
        CCLabelBMFont* loadingLabel = nullptr;
    };

    struct Fields {
        bool m_confirmedLoggedOutPlay = false;
        std::shared_ptr<LevelInfoRequestState> m_levelInfoRequest;
    };

    void cancelLevelInfoRequest() {
        auto state = m_fields->m_levelInfoRequest;
        if (!state) {
            return;
        }

        state->active = false;
        if (state->loadingLabel && state->loadingLabel->getParent()) {
            state->loadingLabel->removeFromParent();
        }
    }

    bool init(GJGameLevel* level, bool a) {
        if (!LevelInfoLayer::init(level, a)) {
            return false;
        }

        int id = level->m_levelID.value();
        int coinCount = level->m_coins;
        auto showLevelInfo = Mod::get()->getSettingValue<bool>("show-level-info");

        if (showLevelInfo) {
            auto loadingLabel = gdvn::level_info::LevelInfoLayerFactory::createLabel(level, "...", 0);
            loadingLabel->retain();

            this->addChild(loadingLabel);

            bool isLoggedIn = AuthService::isLoggedIn();
            auto state = std::make_shared<LevelInfoRequestState>();
            state->loadingLabel = loadingLabel;
            m_fields->m_levelInfoRequest = state;

            auto layer = this;
            layer->retain();

            LevelService::getLevel(id, [id, coinCount, isLoggedIn, layer, state](LevelInfoResponseDto const& model,
                                                                                    web::WebResponse& res) {
                auto cleanup = [layer, state]() {
                    if (state->loadingLabel) {
                        if (state->loadingLabel->getParent()) {
                            state->loadingLabel->removeFromParent();
                        }
                        state->loadingLabel->release();
                        state->loadingLabel = nullptr;
                    }

                    if (layer->m_fields->m_levelInfoRequest == state) {
                        layer->m_fields->m_levelInfoRequest.reset();
                    }

                    layer->release();
                };

                if (!state->active || !layer->isRunning()) {
                    cleanup();
                    return;
                }

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
                    auto btn = gdvn::level_info::LevelInfoLayerFactory::createButton(
                        labels, id, coinCount, layer, menu_selector(GDVNLevelInfoLayer::onGDVNLevelInfo));

                    layer->addChild(btn);
                }

                cleanup();
            });
        }

        return true;
    }

    void onExit() {
        this->cancelLevelInfoRequest();
        LevelInfoLayer::onExit();
    }

    void onGDVNLevelInfo(CCObject* sender) {
        int id = sender->getTag();
        std::string url = "https://www.gdvn.net/level/" + std::to_string(id);

        web::openLinkInBrowser(url);
    }

    void onPlay(CCObject* sender) {
        if (!AuthService::isLoggedIn() && !m_fields->m_confirmedLoggedOutPlay) {
            this->retain();
            if (sender) {
                sender->retain();
            }

            geode::createQuickPopup("GDVN", "You are not logged in, progress will not be saved to GDVN server.",
                                    "Cancel", "Play", [this, sender](auto, bool btn2) {
                                        if (btn2) {
                                            m_fields->m_confirmedLoggedOutPlay = true;
                                            LevelInfoLayer::onPlay(sender);
                                            m_fields->m_confirmedLoggedOutPlay = false;
                                        }

                                        if (sender) {
                                            sender->release();
                                        }
                                        this->release();
                                    });
            return;
        }

        LevelInfoLayer::onPlay(sender);
    }
};
