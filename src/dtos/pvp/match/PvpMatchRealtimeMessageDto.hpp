#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>
#include <string>

struct PvpMatchRealtimeMessageDto {
    bool valid = false;
    std::string event;
    bool replyOk = false;
    std::string table;
    matjson::Value row;
    std::int64_t rowMatchID = 0;
    std::int64_t rowID = 0;
};
