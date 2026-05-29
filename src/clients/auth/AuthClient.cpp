#include "AuthClient.hpp"

#include "../../adapters/AuthMeResponseAdapter.hpp"
#include "../../adapters/OtpGrantResponseAdapter.hpp"
#include "../../adapters/OtpResponseAdapter.hpp"
#include "../../adapters/RealtimeTokenResponseAdapter.hpp"
#include "../../consts/ConfigConst.hpp"

async::TaskHolder<web::WebResponse> AuthClient::s_postHolder;
async::TaskHolder<web::WebResponse> AuthClient::s_getHolder;

void AuthClient::postOTP(PostOTPCallback callback) {
    web::WebRequest req;

    AuthClient::s_postHolder.spawn(req.post(gdvn::config::API_URL + "/auth/otp"), [callback](web::WebResponse res) {
        OtpResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = OtpResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}

void AuthClient::getOTP(std::string const& code, GetOTPCallback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/auth/otp/" + code;

    AuthClient::s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        OtpGrantResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = OtpGrantResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}

void AuthClient::deleteAPIKey(Callback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/APIKey";

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    AuthClient::s_postHolder.spawn(req.send("DELETE", url), [callback](web::WebResponse res) {
        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void AuthClient::getMe(GetMeCallback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/auth/me";

    req.header("Authorization", "Bearer " + gdvn::config::getToken());
    req.header("X-GDVN-Mod-Version", Mod::get()->getVersion().toNonVString());

    AuthClient::s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        AuthMeResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = AuthMeResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}

void AuthClient::getRealtimeToken(GetRealtimeTokenCallback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/auth/realtime-token";

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    AuthClient::s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        RealtimeTokenResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = RealtimeTokenResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}
