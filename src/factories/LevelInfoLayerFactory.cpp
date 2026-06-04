#include "LevelInfoLayerFactory.hpp"

#include "../utils/StringUtils.hpp"

using namespace geode::prelude;

namespace gdvn::level_info {

CCLabelBMFont* LevelInfoLayerFactory::createLabel(GJGameLevel* level, std::string const& text, int order) {
    int offset = (level->m_coins == 0) ? 17 : 4;
    auto size = CCDirector::sharedDirector()->getWinSize();
    int yoffset = 2;

    std::string safeText = utils::string::toAsciiCompatible(text);
    CCLabelBMFont* label = CCLabelBMFont::create(safeText.c_str(), "goldFont.fnt");

    label->setPosition({size.width / 2 - 100, size.height / 2 + offset + yoffset + order * -10});
    label->setScale(0.3f);

    return label;
}

CCMenu* LevelInfoLayerFactory::createButton(std::vector<std::string> const& labels, int levelID, int coinCount,
                                            CCObject* target, SEL_MenuHandler callback) {
    int offset = (coinCount == 0) ? 17 : 4;
    auto size = CCDirector::sharedDirector()->getWinSize();

    std::string text;

    for (std::string const& label : labels) {
        text += label;
        text += '\n';
    }

    if (!text.empty()) {
        text.pop_back();
    }

    std::string safeText = utils::string::toAsciiCompatible(text);
    CCLabelBMFont* label = CCLabelBMFont::create(safeText.c_str(), "goldFont.fnt");

    label->setScale(0.3f);
    label->setAlignment(kCCTextAlignmentCenter);

    auto btn = CCMenuItemSpriteExtra::create(label, target, callback);

    btn->setTag(levelID);

    auto menu = CCMenu::create();

    menu->addChild(btn);
    menu->setPosition({size.width / 2 - 100, size.height / 2 + offset});

    return menu;
}

} // namespace gdvn::level_info
