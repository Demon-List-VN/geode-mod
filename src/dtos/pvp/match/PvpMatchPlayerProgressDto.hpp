#pragma once

#include <string>

struct PvpMatchPlayerProgressDto {
    bool valid = false;
    std::string uid;
    std::string name;
    float progress = 0.0f;
};
