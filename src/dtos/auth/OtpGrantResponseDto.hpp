#pragma once

#include <string>

struct OtpGrantResponseDto {
    bool valid = false;
    bool granted = false;
    std::string key;
    std::string player;
};
