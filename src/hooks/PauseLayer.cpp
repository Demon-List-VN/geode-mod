#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/modify/PauseLayer.hpp> // DO NOT REMOVE

#include "../services/PvpOverlay.hpp"

using namespace geode::prelude;

class $modify(GDVNPauseLayer, PauseLayer) {
	void customSetup() override {
		PauseLayer::customSetup();

		auto overlay = PvpOverlay::getActive();
		if (!overlay || !overlay->hasPvpMatch()) {
			return;
		}

		auto size = CCDirector::sharedDirector()->getWinSize();
		auto menu = CCMenu::create();
		menu->setID("gdvn-pvp-chat-pause-menu"_spr);
		menu->setPosition({ 0.0f, 0.0f });
		this->addChild(menu, 100);
		this->setKeyboardEnabled(true);

		auto chatSprite = ButtonSprite::create("Chat", "goldFont.fnt", "GJ_button_01.png", 0.8f);
		chatSprite->setScale(0.55f);
		auto chatButton = CCMenuItemExt::createSpriteExtra(chatSprite, [](auto*) {
			if (auto overlay = PvpOverlay::getActive()) {
				overlay->openChat();
			}
		});
		chatButton->setID("gdvn-pvp-chat-button"_spr);
		menu->addChildAtPosition(chatButton, Anchor::Center, { 40.0f, size.height / 2.0f + 28.0f });

		this->createToggleButton(
			"Mute Chat",
			menu_selector(GDVNPauseLayer::onGDVNMuteChat),
			overlay->isChatMuted(),
			menu,
			{ 40.0f, size.height / 2.0f }
		);
	}

	void onGDVNMuteChat(CCObject*) {
		auto overlay = PvpOverlay::getActive();
		if (!overlay) {
			auto muted = !Mod::get()->getSavedValue<bool>("pvp-chat-muted", false);
			Mod::get()->setSavedValue<bool>("pvp-chat-muted", muted);
			return;
		}

		overlay->setChatMuted(!overlay->isChatMuted());
	}

	void keyDown(enumKeyCodes key, double timestamp) override {
		if (key == KEY_Enter || key == KEY_NumEnter) {
			if (auto overlay = PvpOverlay::getActive()) {
				if (overlay->openChat()) {
					return;
				}
			}
		}

		PauseLayer::keyDown(key, timestamp);
	}
};
