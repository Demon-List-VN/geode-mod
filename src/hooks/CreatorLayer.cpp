#include <Geode/Geode.hpp>
#include <Geode/binding/CreateMenuItem.hpp>
#include <Geode/modify/CreateMenuItem.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class $modify(GDVNCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) {
            return false;
        }

        auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(this->getChildByIDRecursive("versus-button"));

        if (!btn) {
            return true;
        }

        btn->setSprite(CCSprite::createWithSpriteFrameName("GJ_versusBtn_001.png"));
        btn->setScale(0.8f);

        return true;
    }

    void onMultiplayer(CCObject*) {
        web::openLinkInBrowser("https://gdvn.net/versus");
    }
};
