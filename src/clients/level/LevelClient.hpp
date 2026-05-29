#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

#include "../../dtos/level/LevelInfoResponseDto.hpp"
#include "../../dtos/pvp/ActivePvpMatchResponseDto.hpp"

using namespace geode::prelude;

class LevelClient {
  public:
    using GetLevelCallback = std::function<void(LevelInfoResponseDto const&, web::WebResponse&)>;
    using GetActivePvpMatchCallback = std::function<void(ActivePvpMatchResponseDto const&, web::WebResponse&)>;

    static void getLevel(int id, GetLevelCallback callback);
    static void getActivePvpMatch(int levelID, GetActivePvpMatchCallback callback);
};
