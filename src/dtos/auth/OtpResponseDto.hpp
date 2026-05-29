#pragma once

#include <string>

struct OtpResponseDto {
    bool valid = false;
    std::string code;
};
