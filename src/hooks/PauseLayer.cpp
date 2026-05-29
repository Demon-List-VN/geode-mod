#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/modify/PauseLayer.hpp> // DO NOT REMOVE
#include <Geode/loader/SettingV3.hpp>

#include "../services/PvpOverlay.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
	listenForKeybindSettingPresses("open-pvp-chat", [](Keybind const&, bool down, bool repeat, double) {
		if (!down || repeat) {
			return false;
		}

		auto overlay = PvpOverlay::getActive();
		return overlay && overlay->openChat();
	});
}

class $modify(GDVNPauseLayer, PauseLayer) {
	void customSetup() override {
		PauseLayer::customSetup();

		auto overlay = PvpOverlay::getActive();
		if (!overlay || !overlay->hasPvpMatch()) {
			return;
		}

		auto menu = typeinfo_cast<CCMenu*>(this->getChildByIDRecursive("right-button-menu"));
		if (!menu) {
			log::warn("Could not find PauseLayer right-button-menu for GDVN controls");
			return;
		}

		auto chatSprite = ButtonSprite::create("Chat", "goldFont.fnt", "GJ_button_01.png", 0.8f);
		chatSprite->setScale(0.55f);
		auto chatButton = CCMenuItemExt::createSpriteExtra(chatSprite, [](auto*) {
			if (auto overlay = PvpOverlay::getActive()) {
				overlay->openChat();
			}
		});
		chatButton->setID("gdvn-pvp-chat-button"_spr);
		menu->addChild(chatButton);

		this->createToggleButton(
			"Mute Chat",
			menu_selector(GDVNPauseLayer::onGDVNMuteChat),
			overlay->isChatMuted(),
			menu,
			{ 0.0f, 0.0f }
		);

		menu->updateLayout();
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
};
