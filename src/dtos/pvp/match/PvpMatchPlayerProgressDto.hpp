#pragma once

#include <string>

struct PvpMatchPlayerProgressDto {
    bool valid = false;
    std::string uid;
    float progress = 0.0f;
};
