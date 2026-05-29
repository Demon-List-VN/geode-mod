#pragma once

#include <cstdint>
#include <string>

struct RealtimeTokenResponseDto {
    bool valid = false;
    std::string supabaseUrl;
    std::string anonKey;
    std::string accessToken;
    std::int64_t expiresAt = 0;
};
