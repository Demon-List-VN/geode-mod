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
    std::string skill;
    std::string casterUid;
    std::string targetUid;
    std::string revealAt;
    std::string payloadEffect;
    std::string payloadExpiresAt;
    std::string payloadSuppressedUntil;
    int payloadRequiredClicks = 0;
    int durationMs = 0;
    int payloadDurationMs = 0;
    std::int64_t nextLevelID = 0;
};
