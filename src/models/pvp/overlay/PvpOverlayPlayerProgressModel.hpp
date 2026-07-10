#pragma once

#include <string>

struct PvpOverlayPlayerProgressModel {
    std::string uid;
    std::string name;
    float progress = 0.0f;
    float progressSpeed = 0.0f;
    float deathProgress = -1.0f;
    bool progressRunning = false;
    std::string playMode = "normal";
};
