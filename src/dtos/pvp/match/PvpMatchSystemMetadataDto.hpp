#pragma once

#include <cstdint>
#include <string>

struct PvpMatchSystemMetadataDto {
    bool valid = false;
    std::string kind;
    std::string uid;
    std::string playMode;
    float progress = 0.0f;
    float damage = 0.0f;
    std::string mode;
    std::string scoringMode = "progress";
    int targetScore = 0;
    int startingHp = 0;
    int finalizeAliveCount = 0;
    std::string winnerUid;
    std::string resigningUid;
    std::string requesterUid;
    std::int64_t nextLevelID = 0;
};
