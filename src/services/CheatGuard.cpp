#include "CheatGuard.hpp"

#include <array>
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/utils/general.hpp>
#include <legowiifun.cheat_api/include/cheatAPI.hpp>

namespace {
	bool isQolModModuleEnabled(geode::Mod* mod, std::string const& id, bool defaultValue = false) {
		return mod->getSavedValue<bool>(id + "_enabled", defaultValue);
	}

	float getQolModSpeedhackValue(geode::Mod* mod) {
		auto value = mod->getSavedValue<std::string>("speedhack-top_value", "1.0");
		return geode::utils::numFromString<float>(value).unwrapOr(1.0f);
	}

	bool isQolModCheated() {
		auto mod = geode::Loader::get()->getLoadedMod("thesillydoggo.qolmod");

		if (!mod) {
			return false;
		}

		if (isQolModModuleEnabled(mod, "safe-mode")) {
			return true;
		}

		if (auto playLayer = PlayLayer::get(); playLayer && playLayer->m_startPosObject) {
			return true;
		}

		bool disablesCheatsInUi = isQolModModuleEnabled(mod, "safe-mode/disable-cheats-ui");

		static constexpr std::array levelLoadModules = {
			"force-plat",
			"accurate-spike-hitboxes",
			"show-triggers",
			"show-layout",
		};

		static constexpr std::array attemptModules = {
			"instant",
			"no-reverse",
			"instant-reverse",
			"no-static",
			"show-hitboxes",
			"coin-tracers",
			"show-trajectory",
			"rand-seed",
			"jump-hack",
			"tps-bypass",
			"auto-coins",
			"autoclicker",
			"auto-clicker",
			"frame-stepper",
			"hitbox-multiplier",
			"legacy-upside-down",
			"no-invisible-objects",
			"gravity-arrow",
			"show-player",
			"noclip",
		};

		if (!disablesCheatsInUi) {
			for (auto id : levelLoadModules) {
				if (isQolModModuleEnabled(mod, id)) {
					return true;
				}
			}

			for (auto id : attemptModules) {
				if (isQolModModuleEnabled(mod, id)) {
					return true;
				}
			}
		}

		if (
			(isQolModModuleEnabled(mod, "speedhack/enabled") || isQolModModuleEnabled(mod, "speedhack-enabled")) &&
			getQolModSpeedhackValue(mod) < 1.0f
		) {
			return true;
		}

		return false;
	}
}

bool CheatGuard::isGameplayCheated() {
	static constexpr std::array rulesetsToCheck = {
		ROBTOP,
		DEMONLIST,
		GDDL,
		MODMAKEROPINION,
		AREDL,
		PEMONLIST,
	};

	for (auto ruleset : rulesetsToCheck) {
		if (cheatAPI::isCheating(ruleset)) {
			return true;
		}
	}

	if (isQolModCheated()) {
		return true;
	}

	return false;
}
