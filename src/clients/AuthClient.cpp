#include "AuthClient.hpp"

#include "../common.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_postHolder;
async::TaskHolder<web::WebResponse> s_getHolder;
}

static std::string getToken() {
	return Mod::get()->getSavedValue<std::string>("api-key");
}

void AuthClient::postOTP(Callback callback) {
	web::WebRequest req;

	s_postHolder.spawn(req.post(API_URL + "/auth/otp"), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::getOTP(std::string const& code, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/otp/" + code;

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::deleteAPIKey(Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/APIKey";

	req.header("Authorization", "Bearer " + getToken());

	s_postHolder.spawn(req.send("DELETE", url), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::getMe(Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/me";

	req.header("Authorization", "Bearer " + getToken());
	req.header("X-GDVN-Mod-Version", Mod::get()->getVersion().toNonVString());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::getRealtimeToken(Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/realtime-token";

	req.header("Authorization", "Bearer " + getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}
