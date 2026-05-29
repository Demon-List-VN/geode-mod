#include "AuthClient.hpp"

#include "../common.hpp"

async::TaskHolder<web::WebResponse> AuthClient::m_post_holder;
async::TaskHolder<web::WebResponse> AuthClient::m_get_holder;

void AuthClient::requestOTP(Callback callback) {
	web::WebRequest req;

	m_post_holder.spawn(req.post(API_URL + "/auth/otp"), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::checkOTP(std::string const& code, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/otp/" + code;

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::logout(std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/APIKey";

	req.header("Authorization", "Bearer " + token);

	m_post_holder.spawn(req.send("DELETE", url), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::checkMe(std::string const& token, std::string const& modVersion, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/me";

	req.header("Authorization", "Bearer " + token);
	req.header("X-GDVN-Mod-Version", modVersion);

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void AuthClient::getRealtimeToken(std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/realtime-token";

	req.header("Authorization", "Bearer " + token);

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}
