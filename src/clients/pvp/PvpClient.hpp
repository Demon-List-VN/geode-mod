#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../dtos/common/EmptyResponseDto.hpp"
#include "../../dtos/pvp/PvpMessageDto.hpp"
#include "../../dtos/pvp/PvpMessagesResponseDto.hpp"
#include "../../dtos/pvp/PvpPowerupCastResponseDto.hpp"
#include "../../dtos/pvp/PvpPowerupStateDto.hpp"
#include "../../dtos/pvp/match/PvpMatchSnapshotDto.hpp"

using namespace geode::prelude;

class PvpClient {
  public:
    using Callback = std::function<void(EmptyResponseDto const&, web::WebResponse&)>;
    using GetMatchCallback = std::function<void(PvpMatchSnapshotDto const&, web::WebResponse&)>;
    using GetMessagesCallback = std::function<void(PvpMessagesResponseDto const&, web::WebResponse&)>;
    using PostMessageCallback = std::function<void(PvpMessageDto const&, web::WebResponse&)>;
    using GetPowerupStateCallback = std::function<void(PvpPowerupStateDto const&, web::WebResponse&)>;
    using CastPowerupCallback = std::function<void(PvpPowerupCastResponseDto const&, web::WebResponse&)>;

    static void getMatch(int matchID, GetMatchCallback callback);
    static void putPlayMode(int matchID, std::string const& playMode, Callback callback);
    static void putProgress(int matchID, float progress, bool completed, Callback callback,
                            std::string const& attemptID = "");
    static void postProgressRun(int matchID, float progress, float progressSpeed, std::string const& attemptID,
                                Callback callback);
    static void postDeathProgress(int matchID, float progress, Callback callback,
                                  std::string const& attemptID = "");
    static void postDeathCount(int matchID, std::string const& count, Callback callback);
    static void getMessages(int matchID, std::int64_t afterID, int limit, GetMessagesCallback callback);
    static void postMessage(int matchID, std::string const& content, PostMessageCallback callback);
    static void getPowerupState(int matchID, GetPowerupStateCallback callback);
    static void castPowerup(int matchID,
                            std::string const& skill,
                            std::string const& targetUid,
                            bool randomTarget,
                            CastPowerupCallback callback);
  private:
    static async::TaskHolder<web::WebResponse> s_putPlayModeHolder;
    static async::TaskHolder<web::WebResponse> s_putProgressHolder;
    static std::vector<std::shared_ptr<async::TaskHolder<web::WebResponse>>> s_postHolders;
    static std::vector<std::shared_ptr<async::TaskHolder<web::WebResponse>>> s_getMatchHolders;
    static async::TaskHolder<web::WebResponse> s_getHolder;
};
