#pragma once

#include <string>

struct PvpOverlayPlayerProgressModel {
    std::string uid;
    std::string name;
    float progress = 0.0f;
    std::string playMode = "normal";
};
