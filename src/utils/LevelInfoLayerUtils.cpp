#include "LevelInfoLayerUtils.hpp"
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace gdvn::level_info {

std::string replaceAll(std::string text, std::string const& from, std::string const& to) {
    size_t pos = 0;

    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }

    return text;
}

std::string toAsciiCompatible(std::string text) {
    std::vector<std::pair<std::string, std::string>> replacements = {
        {"\xC3\xA0", "a"},     {"\xC3\xA1", "a"},     {"\xE1\xBA\xA3", "a"}, {"\xC3\xA3", "a"},
        {"\xE1\xBA\xA1", "a"}, {"\xC4\x83", "a"},     {"\xE1\xBA\xB1", "a"}, {"\xE1\xBA\xAF", "a"},
        {"\xE1\xBA\xB3", "a"}, {"\xE1\xBA\xB5", "a"}, {"\xE1\xBA\xB7", "a"}, {"\xC3\xA2", "a"},
        {"\xE1\xBA\xA7", "a"}, {"\xE1\xBA\xA5", "a"}, {"\xE1\xBA\xA9", "a"}, {"\xE1\xBA\xAB", "a"},
        {"\xE1\xBA\xAD", "a"}, {"\xC3\x80", "A"},     {"\xC3\x81", "A"},     {"\xE1\xBA\xA2", "A"},
        {"\xC3\x83", "A"},     {"\xE1\xBA\xA0", "A"}, {"\xC4\x82", "A"},     {"\xE1\xBA\xB0", "A"},
        {"\xE1\xBA\xAE", "A"}, {"\xE1\xBA\xB2", "A"}, {"\xE1\xBA\xB4", "A"}, {"\xE1\xBA\xB6", "A"},
        {"\xC3\x82", "A"},     {"\xE1\xBA\xA6", "A"}, {"\xE1\xBA\xA4", "A"}, {"\xE1\xBA\xA8", "A"},
        {"\xE1\xBA\xAA", "A"}, {"\xE1\xBA\xAC", "A"}, {"\xC4\x91", "d"},     {"\xC4\x90", "D"},
        {"\xC3\xA8", "e"},     {"\xC3\xA9", "e"},     {"\xE1\xBA\xBB", "e"}, {"\xE1\xBA\xBD", "e"},
        {"\xE1\xBA\xB9", "e"}, {"\xC3\xAA", "e"},     {"\xE1\xBB\x81", "e"}, {"\xE1\xBA\xBF", "e"},
        {"\xE1\xBB\x83", "e"}, {"\xE1\xBB\x85", "e"}, {"\xE1\xBB\x87", "e"}, {"\xC3\x88", "E"},
        {"\xC3\x89", "E"},     {"\xE1\xBA\xBA", "E"}, {"\xE1\xBA\xBC", "E"}, {"\xE1\xBA\xB8", "E"},
        {"\xC3\x8A", "E"},     {"\xE1\xBB\x80", "E"}, {"\xE1\xBA\xBE", "E"}, {"\xE1\xBB\x82", "E"},
        {"\xE1\xBB\x84", "E"}, {"\xE1\xBB\x86", "E"}, {"\xC3\xAC", "i"},     {"\xC3\xAD", "i"},
        {"\xE1\xBB\x89", "i"}, {"\xC4\xA9", "i"},     {"\xE1\xBB\x8B", "i"}, {"\xC3\x8C", "I"},
        {"\xC3\x8D", "I"},     {"\xE1\xBB\x88", "I"}, {"\xC4\xA8", "I"},     {"\xE1\xBB\x8A", "I"},
        {"\xC3\xB2", "o"},     {"\xC3\xB3", "o"},     {"\xE1\xBB\x8F", "o"}, {"\xC3\xB5", "o"},
        {"\xE1\xBB\x8D", "o"}, {"\xC3\xB4", "o"},     {"\xE1\xBB\x93", "o"}, {"\xE1\xBB\x91", "o"},
        {"\xE1\xBB\x95", "o"}, {"\xE1\xBB\x97", "o"}, {"\xE1\xBB\x99", "o"}, {"\xC6\xA1", "o"},
        {"\xE1\xBB\x9D", "o"}, {"\xE1\xBB\x9B", "o"}, {"\xE1\xBB\x9F", "o"}, {"\xE1\xBB\xA1", "o"},
        {"\xE1\xBB\xA3", "o"}, {"\xC3\x92", "O"},     {"\xC3\x93", "O"},     {"\xE1\xBB\x8E", "O"},
        {"\xC3\x95", "O"},     {"\xE1\xBB\x8C", "O"}, {"\xC3\x94", "O"},     {"\xE1\xBB\x92", "O"},
        {"\xE1\xBB\x90", "O"}, {"\xE1\xBB\x94", "O"}, {"\xE1\xBB\x96", "O"}, {"\xE1\xBB\x98", "O"},
        {"\xC6\xA0", "O"},     {"\xE1\xBB\x9C", "O"}, {"\xE1\xBB\x9A", "O"}, {"\xE1\xBB\x9E", "O"},
        {"\xE1\xBB\xA0", "O"}, {"\xE1\xBB\xA2", "O"}, {"\xC3\xB9", "u"},     {"\xC3\xBA", "u"},
        {"\xE1\xBB\xA7", "u"}, {"\xC5\xA9", "u"},     {"\xE1\xBB\xA5", "u"}, {"\xC6\xB0", "u"},
        {"\xE1\xBB\xAB", "u"}, {"\xE1\xBB\xA9", "u"}, {"\xE1\xBB\xAD", "u"}, {"\xE1\xBB\xAF", "u"},
        {"\xE1\xBB\xB1", "u"}, {"\xC3\x99", "U"},     {"\xC3\x9A", "U"},     {"\xE1\xBB\xA6", "U"},
        {"\xC5\xA8", "U"},     {"\xE1\xBB\xA4", "U"}, {"\xC6\xAF", "U"},     {"\xE1\xBB\xAA", "U"},
        {"\xE1\xBB\xA8", "U"}, {"\xE1\xBB\xAC", "U"}, {"\xE1\xBB\xAE", "U"}, {"\xE1\xBB\xB0", "U"},
        {"\xE1\xBB\xB3", "y"}, {"\xC3\xBD", "y"},     {"\xE1\xBB\xB7", "y"}, {"\xE1\xBB\xB9", "y"},
        {"\xE1\xBB\xB5", "y"}, {"\xE1\xBB\xB2", "Y"}, {"\xC3\x9D", "Y"},     {"\xE1\xBB\xB6", "Y"},
        {"\xE1\xBB\xB8", "Y"}, {"\xE1\xBB\xB4", "Y"},
    };

    for (auto const& [from, to] : replacements) {
        text = replaceAll(text, from, to);
    }

    std::string ascii;

    for (unsigned char c : text) {
        if (c < 128) {
            ascii += static_cast<char>(c);
        }
    }

    return ascii;
}

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

CCLabelBMFont* createLabel(GJGameLevel* level, std::string str, int order) {
    int offset = (level->m_coins == 0) ? 17 : 4;
    auto size = CCDirector::sharedDirector()->getWinSize();
    int yoffset = 2;

    std::string safeText = toAsciiCompatible(str);
    CCLabelBMFont* label = CCLabelBMFont::create(safeText.c_str(), "goldFont.fnt");

    label->setPosition({size.width / 2 - 100, size.height / 2 + offset + yoffset + order * -10});
    label->setScale(0.3f);

    return label;
}

void ButtonCreator::onButton(CCObject* sender) {
    int id = sender->getTag();
    std::string url = "https://www.gdvn.net/level/" + std::to_string(id);

    web::openLinkInBrowser(url);
}

CCMenu* ButtonCreator::create(std::vector<std::string> labels, GJGameLevel* level, CCLayer* layer) {
    int offset = (level->m_coins == 0) ? 17 : 4;
    auto size = CCDirector::sharedDirector()->getWinSize();

    std::string text;

    for (std::string& s : labels) {
        text += s;
        text += '\n';
    }

    text.pop_back();

    std::string safeText = toAsciiCompatible(text);
    CCLabelBMFont* label = CCLabelBMFont::create(safeText.c_str(), "goldFont.fnt");

    label->setScale(0.3f);
    label->setAlignment(kCCTextAlignmentCenter);

    auto btn = CCMenuItemSpriteExtra::create(label, layer, menu_selector(ButtonCreator::onButton));

    btn->setTag(level->m_levelID.value());

    auto menu = CCMenu::create();

    menu->addChild(btn);
    menu->setPosition({size.width / 2 - 100, size.height / 2 + offset});

    return menu;
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

} // namespace gdvn::level_info
