#include "CheatGuardService.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/loader/Event.hpp>

#ifdef GEODE_IS_WINDOWS
#include <Geode/platform/windows.hpp>
#endif

namespace eclipse {
	struct HackingModule {
		std::string_view name;
		enum class State {
			Enabled,
			Tripped,
		} state = State::Enabled;
	};
}

namespace eclipse::__internal__ {
	constexpr size_t API_VERSION = 1;

	struct VTable {
		bool (*Config_getBool)(std::string_view, bool const&) = nullptr;
		int (*Config_getInt)(std::string_view, int const&) = nullptr;
		double (*Config_getDouble)(std::string_view, double const&) = nullptr;
		std::string (*Config_getString)(std::string_view, std::string const&) = nullptr;
		std::string_view (*Config_getStringView)(std::string_view, std::string_view const&) = nullptr;
		bool (*Config_getBoolInternal)(std::string_view, bool const&) = nullptr;
		int (*Config_getIntInternal)(std::string_view, int const&) = nullptr;
		double (*Config_getDoubleInternal)(std::string_view, double const&) = nullptr;
		std::string (*Config_getStringInternal)(std::string_view, std::string const&) = nullptr;
		std::string_view (*Config_getStringViewInternal)(std::string_view, std::string_view const&) = nullptr;

		void (*Config_setBool)(std::string_view, bool) = nullptr;
		void (*Config_setInt)(std::string_view, int) = nullptr;
		void (*Config_setDouble)(std::string_view, double) = nullptr;
		void (*Config_setString)(std::string_view, std::string) = nullptr;
		void (*Config_setStringView)(std::string_view, std::string_view) = nullptr;
		void (*Config_setBoolInternal)(std::string_view, bool) = nullptr;
		void (*Config_setIntInternal)(std::string_view, int) = nullptr;
		void (*Config_setDoubleInternal)(std::string_view, double) = nullptr;
		void (*Config_setStringInternal)(std::string_view, std::string) = nullptr;
		void (*Config_setStringViewInternal)(std::string_view, std::string_view) = nullptr;

		std::string (*FormatRiftString)(std::string_view) = nullptr;
		geode::Result<std::nullptr_t> (*GetRiftVariableNull)(std::string_view) = nullptr;
		geode::Result<bool> (*GetRiftVariableBool)(std::string_view) = nullptr;
		geode::Result<int64_t> (*GetRiftVariableInt)(std::string_view) = nullptr;
		geode::Result<double> (*GetRiftVariableDouble)(std::string_view) = nullptr;
		geode::Result<std::string> (*GetRiftVariableString)(std::string_view) = nullptr;
		geode::Result<matjson::Value> (*GetRiftVariableObject)(std::string_view) = nullptr;
		void (*SetRiftVariableNull)(std::string, std::nullptr_t) = nullptr;
		void (*SetRiftVariableBool)(std::string, bool) = nullptr;
		void (*SetRiftVariableInt)(std::string, int64_t) = nullptr;
		void (*SetRiftVariableDouble)(std::string, double) = nullptr;
		void (*SetRiftVariableString)(std::string, std::string) = nullptr;
		void (*SetRiftVariableObject)(std::string, matjson::Value) = nullptr;

		void (*RegisterCheat)(std::string, geode::Function<bool()>) = nullptr;
		geode::Result<> (*LoadReplay)(std::filesystem::path const&) = nullptr;
		geode::Result<> (*LoadReplayFromData)(std::span<uint8_t>) = nullptr;
		bool (*CheckCheatsEnabled)() = nullptr;
		bool (*CheckCheatedInAttempt)() = nullptr;
		std::vector<HackingModule> (*GetHackingModules)() = nullptr;

		void (*CreateMenuTab)(std::string_view) = nullptr;
		size_t (*CreateLabel)(std::string_view, std::string) = nullptr;
		size_t (*CreateToggle)(std::string_view, std::string, std::string, geode::Function<void(bool)>) = nullptr;
		size_t (*CreateInputFloat)(std::string_view, std::string, std::string, geode::Function<void(float)>) = nullptr;
		size_t (*CreateButton)(std::string_view, std::string, geode::Function<void()>) = nullptr;
		void (*SetComponentDescription)(size_t, std::string) = nullptr;
		void (*SetLabelText)(size_t, std::string) = nullptr;
		void (*SetInputFloatParams)(size_t, std::optional<float>, std::optional<float>, std::optional<std::string>) = nullptr;
	};

	struct FetchVTableEvent : geode::Event<FetchVTableEvent, bool(VTable&, size_t)> {
		using Event::Event;
	};

	VTable const& getVTable() {
		static VTable vtable{};
		static bool initialized = false;

		if (!initialized) {
			initialized = FetchVTableEvent().send(vtable, API_VERSION);
		}

		return vtable;
	}
}

namespace gdvn::cheat_guard_detail {
	constexpr float FLOAT_EPSILON = 0.001f;
	constexpr bool ENABLE_CONFIG_BASED_CHEAT_CHECKS = true;

	geode::Mod* getLoadedMod(std::string_view id) {
		return geode::Loader::get()->getLoadedMod(id);
	}

	bool differsFrom(float value, float expected) {
		return std::abs(value - expected) > FLOAT_EPSILON;
	}

	bool isStartPositionRun() {
		auto playLayer = PlayLayer::get();
		return playLayer && playLayer->m_startPosObject;
	}

	bool isQolModModuleEnabled(geode::Mod* mod, std::string const& id, bool defaultValue = false) {
		return mod->getSavedValue<bool>(id + "_enabled", defaultValue);
	}

	bool isQolModCheated() {
		auto mod = getLoadedMod("thesillydoggo.qolmod");

		if (!mod) {
			return false;
		}

		if (isQolModModuleEnabled(mod, "safe-mode")) {
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
			"hide-player",
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

		return false;
	}

	std::optional<matjson::Value> getPrismSavedValue(std::vector<matjson::Value> const& values, std::string_view name) {
		for (auto const& item : values) {
			if (!item.isObject()) {
				continue;
			}

			if (item["name"].asString().unwrapOrDefault() == name) {
				return item["value"];
			}
		}

		return std::nullopt;
	}

	bool getPrismSavedBool(std::vector<matjson::Value> const& values, std::string_view name) {
		auto value = getPrismSavedValue(values, name);
		return value && value->asBool().unwrapOr(false);
	}

	float getPrismSavedFloat(std::vector<matjson::Value> const& values, std::string_view name, float defaultValue) {
		auto value = getPrismSavedValue(values, name);
		if (!value) {
			return defaultValue;
		}

		return static_cast<float>(value->asDouble().unwrapOr(defaultValue));
	}

	bool isPrismCheated() {
		auto mod = getLoadedMod("firee.prism");
		if (!mod) {
			return false;
		}

		auto values = mod->getSavedValue<std::vector<matjson::Value>>("values");

		static constexpr std::array boolCheats = {
			"Safe Mode",
			"Anticheat Bypass",
			"Noclip",
			"Instant Complete",
			"No Spikes",
			"No Hitbox",
			"No Solids",
			"No Rotate",
			"Hide Player",
			"Freeze Player",
			"Jump Hack",
			"Layout Mode",
			"Show Hidden Objects",
			"Hide Level",
			"Change Gravity",
			"Force Platformer Mode",
			"No Mirror Transition",
			"Instant Mirror Portal",
			"Show Hitboxes",
			"No Shaders",
			"Playback",
			"Disable Camera Effects",
			"Auto Clicker",
		};

		for (auto cheat : boolCheats) {
			if (getPrismSavedBool(values, cheat)) {
				return true;
			}
		}

		return differsFrom(getPrismSavedFloat(values, "TPS Bypass", 240.0f), 240.0f);
	}

	bool isEclipseCheated() {
		if (!getLoadedMod("eclipse.eclipse-menu")) {
			return false;
		}

		auto const& vtable = eclipse::__internal__::getVTable();

		if (vtable.CheckCheatedInAttempt && vtable.CheckCheatedInAttempt()) {
			return true;
		}

		if (ENABLE_CONFIG_BASED_CHEAT_CHECKS && vtable.CheckCheatsEnabled && vtable.CheckCheatsEnabled()) {
			return true;
		}

		return false;
	}

	bool isOpenHackCheated() {
		if (!getLoadedMod("prevter.openhack")) {
			return false;
		}

#ifdef GEODE_IS_WINDOWS
		auto library = GetModuleHandleA("prevter.openhack.dll");
		if (!library) {
			return false;
		}

		using IsCheating = bool (*)();
		auto isCheating = reinterpret_cast<IsCheating>(GetProcAddress(library, "?isCheating@openhack@@YA_NXZ"));
		return isCheating && isCheating();
#else
		return false;
#endif
	}

	}

using namespace gdvn::cheat_guard_detail;

bool CheatGuardService::isGameplayCheated() {
	return CheatGuardService::getGameplayCheatReason().has_value();
}

std::optional<std::string_view> CheatGuardService::getGameplayCheatReason() {
	if (isStartPositionRun()) {
		return "start position";
	}

	if (isEclipseCheated()) {
		return "Eclipse cheat state";
	}

	if constexpr (ENABLE_CONFIG_BASED_CHEAT_CHECKS) {
		if (isQolModCheated()) {
			return "QOLMod config";
		}

		if (isPrismCheated()) {
			return "Prism config";
		}

	}

	if (isOpenHackCheated()) {
		return "OpenHack runtime state";
	}

	return std::nullopt;
}
