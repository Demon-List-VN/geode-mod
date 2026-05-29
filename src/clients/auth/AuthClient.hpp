#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

#include "../../dtos/auth/AuthMeResponseDto.hpp"
#include "../../dtos/auth/OtpGrantResponseDto.hpp"
#include "../../dtos/auth/OtpResponseDto.hpp"
#include "../../dtos/common/EmptyResponseDto.hpp"
#include "../../dtos/pvp/RealtimeTokenResponseDto.hpp"

using namespace geode::prelude;

class AuthClient {
  public:
    using Callback = std::function<void(EmptyResponseDto const&, web::WebResponse&)>;
    using PostOTPCallback = std::function<void(OtpResponseDto const&, web::WebResponse&)>;
    using GetOTPCallback = std::function<void(OtpGrantResponseDto const&, web::WebResponse&)>;
    using GetMeCallback = std::function<void(AuthMeResponseDto const&, web::WebResponse&)>;
    using GetRealtimeTokenCallback = std::function<void(RealtimeTokenResponseDto const&, web::WebResponse&)>;

    static void postOTP(PostOTPCallback callback);
    static void getOTP(std::string const& code, GetOTPCallback callback);
    static void deleteAPIKey(Callback callback);
    static void getMe(GetMeCallback callback);
    static void getRealtimeToken(GetRealtimeTokenCallback callback);

  private:
    static async::TaskHolder<web::WebResponse> s_postHolder;
    static async::TaskHolder<web::WebResponse> s_getHolder;
};
