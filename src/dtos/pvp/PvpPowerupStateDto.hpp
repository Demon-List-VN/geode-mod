#pragma once

#include <string>
#include <vector>

struct PvpPowerupSkillDto {
    std::string skill;
    int cost = 0;
    int durationMs = 0;
    std::string effect;
    bool harmful = false;
};

struct PvpPowerupPlayerManaDto {
    std::string uid;
    int mana = 0;
    int maxMana = 100;
};

struct PvpPowerupStateDto {
    bool valid = false;
    int matchID = 0;
    std::string uid;
    int mana = 0;
    int maxMana = 100;
    std::string shieldExpiresAt;
    int shieldCharges = 0;
    bool shieldActive = false;
    std::vector<PvpPowerupPlayerManaDto> playerMana;
    std::vector<PvpPowerupSkillDto> skills;
};
