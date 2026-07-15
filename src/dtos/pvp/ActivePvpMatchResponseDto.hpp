#pragma once

#include <Geode/Geode.hpp>
#include <string>

struct ActivePvpMatchResponseDto {
    bool valid = false;
    int matchID = 0;
    int levelID = 0;
    float bestProgress = 0.0f;
    std::string currentUid;
    std::string mode;
    std::string scoringMode = "progress";
    int targetScore = 0;
    int startingHp = 0;
    int finalizeAliveCount = 0;
    std::string status;
    std::string context = "versus";
    std::string roomName;
    matjson::Value rawJson;
};
