#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>

#include "../../dtos/common/EmptyResponseDto.hpp"
#include "../../dtos/github/GithubReleaseResponseDto.hpp"

using namespace geode::prelude;

class UpdaterClient {
  public:
    using Callback = std::function<void(EmptyResponseDto const&, web::WebResponse&)>;
    using GetLatestReleaseCallback = std::function<void(GithubReleaseResponseDto const&, web::WebResponse&)>;

    static void getLatestDownload(Callback callback);
    static void getLatestRelease(GetLatestReleaseCallback callback);

  private:
    static async::TaskHolder<web::WebResponse> s_getHolder;
};
