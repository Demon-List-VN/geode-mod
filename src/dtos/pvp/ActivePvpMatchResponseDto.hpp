#pragma once

#include <Geode/Geode.hpp>
#include <string>

struct ActivePvpMatchResponseDto {
    bool valid = false;
    int matchID = 0;
    int levelID = 0;
    std::string mode;
    std::string context = "versus";
    std::string roomName;
    matjson::Value rawJson;
};
