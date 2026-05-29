#include "AuthClient.hpp"

#include "../../adapters/AuthMeResponseAdapter.hpp"
#include "../../adapters/OtpGrantResponseAdapter.hpp"
#include "../../adapters/OtpResponseAdapter.hpp"
#include "../../adapters/RealtimeTokenResponseAdapter.hpp"
#include "../../common.hpp"
#include "../../utils/AuthConfig.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_postHolder;
async::TaskHolder<web::WebResponse> s_getHolder;
}

void AuthClient::postOTP(PostOTPCallback callback) {
	web::WebRequest req;

	s_postHolder.spawn(req.post(API_URL + "/auth/otp"), [&](web::WebResponse res) {
		OtpResponseDto dto;

		if (res.ok()) {
			auto jsonResult = res.json();
			if (jsonResult) {
				dto = gdvn::adapters::OtpResponseAdapter::fromJson(jsonResult.unwrap());
			}
		}

		callback(dto, res);
	});
}

void AuthClient::getOTP(std::string const& code, GetOTPCallback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/otp/" + code;

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		OtpGrantResponseDto dto;

		if (res.ok()) {
			auto jsonResult = res.json();
			if (jsonResult) {
				dto = gdvn::adapters::OtpGrantResponseAdapter::fromJson(jsonResult.unwrap());
			}
		}

		callback(dto, res);
	});
}

void AuthClient::deleteAPIKey(Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/APIKey";

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_postHolder.spawn(req.send("DELETE", url), [&](web::WebResponse res) {
		EmptyResponseDto dto;
		callback(dto, res);
	});
}

void AuthClient::getMe(GetMeCallback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/me";

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());
	req.header("X-GDVN-Mod-Version", Mod::get()->getVersion().toNonVString());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		AuthMeResponseDto dto;

		if (res.ok()) {
			auto jsonResult = res.json();
			if (jsonResult) {
				dto = gdvn::adapters::AuthMeResponseAdapter::fromJson(jsonResult.unwrap());
			}
		}

		callback(dto, res);
	});
}

void AuthClient::getRealtimeToken(GetRealtimeTokenCallback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/realtime-token";

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		RealtimeTokenResponseDto dto;

		if (res.ok()) {
			auto jsonResult = res.json();
			if (jsonResult) {
				dto = gdvn::adapters::RealtimeTokenResponseAdapter::fromJson(jsonResult.unwrap());
			}
		}

		callback(dto, res);
	});
}
