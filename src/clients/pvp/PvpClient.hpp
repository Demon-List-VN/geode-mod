#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <cstdint>
#include <functional>
#include <string>

#include "../../dtos/common/EmptyResponseDto.hpp"
#include "../../dtos/pvp/PvpMessageDto.hpp"
#include "../../dtos/pvp/PvpMessagesResponseDto.hpp"

using namespace geode::prelude;

class PvpClient {
  public:
    using Callback = std::function<void(EmptyResponseDto const&, web::WebResponse&)>;
    using GetMessagesCallback = std::function<void(PvpMessagesResponseDto const&, web::WebResponse&)>;
    using PostMessageCallback = std::function<void(PvpMessageDto const&, web::WebResponse&)>;

    static void putPlayMode(int matchID, std::string const& playMode, Callback callback);
    static void putProgress(int matchID, float progress, bool completed, Callback callback);
    static void postDeathCount(int matchID, std::string const& count, Callback callback);
    static void getMessages(int matchID, std::int64_t afterID, int limit, GetMessagesCallback callback);
    static void postMessage(int matchID, std::string const& content, PostMessageCallback callback);
};
