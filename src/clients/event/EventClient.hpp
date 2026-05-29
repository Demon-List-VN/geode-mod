#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

#include "../../dtos/common/EmptyResponseDto.hpp"

using namespace geode::prelude;

class EventClient {
  public:
    using Callback = std::function<void(EmptyResponseDto const&, web::WebResponse&)>;

    static void getEventLevel(int levelID, std::string const& type, Callback callback);
    static void putLevel(int levelID, float progress, Callback callback);
};
