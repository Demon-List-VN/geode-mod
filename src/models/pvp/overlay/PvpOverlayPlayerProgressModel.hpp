#pragma once

#include <chrono>
#include <string>

struct PvpOverlayPlayerProgressModel {
    std::string uid;
    std::string name;
    float progress = 0.0f;
    float progressSpeed = 0.0f;
    float deathProgress = -1.0f;
    bool progressRunning = false;
    std::chrono::steady_clock::time_point progressUpdatedAt;
    std::string playMode = "normal";
};
