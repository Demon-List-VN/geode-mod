#pragma once

#include <optional>
#include <string_view>

class CheatGuard {
public:
	static bool isGameplayCheated();
	static std::optional<std::string_view> getGameplayCheatReason();
};
