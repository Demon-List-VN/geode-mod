#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

#include "../../dtos/common/EmptyResponseDto.hpp"

using namespace geode::prelude;

class PvpProgressClient {
  public:
    using Callback = std::function<void(EmptyResponseDto const&, web::WebResponse&)>;

    static void postHeatmap(size_t count, Callback callback);
    static void postDeathCount(int levelID, std::string const& count, bool completed, Callback callback);
};
