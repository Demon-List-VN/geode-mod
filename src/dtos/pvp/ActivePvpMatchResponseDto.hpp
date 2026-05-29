#pragma once

#include <Geode/Geode.hpp>
#include <string>

struct ActivePvpMatchResponseDto {
    bool valid = false;
    int matchID = 0;
    std::string mode;
    matjson::Value rawJson;
};
