#include "LevelClient.hpp"

#include "../common.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
}

static std::string getToken() {
	return Mod::get()->getSavedValue<std::string>("api-key");
}

void LevelClient::getLevel(int id, Callback callback) {
	web::WebRequest req;
	auto token = getToken();

	if (!token.empty()) {
		req.header("Authorization", "Bearer " + token);
	}

	auto url = API_URL + "/lists/levels/" + std::to_string(id) + "/starred";

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void LevelClient::getActivePvpMatch(int levelID, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inPvp";

	req.header("Authorization", "Bearer " + getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void LevelClient::getEventLevel(int levelID, std::string const& type, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent";

	if (!type.empty()) {
		url += "?type=" + type;
	}

	req.header("Authorization", "Bearer " + getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}
