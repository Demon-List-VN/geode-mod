#pragma once

#include <cstdint>
#include <string>

struct PvpMatchSystemMetadataDto {
    bool valid = false;
    std::string kind;
    std::string uid;
    std::string playMode;
    float progress = 0.0f;
    std::string mode;
    std::string winnerUid;
    std::string resigningUid;
    std::string requesterUid;
    std::int64_t nextLevelID = 0;
};
