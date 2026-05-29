#include "LevelClient.hpp"

#include "../../common.hpp"
#include "../../utils/AuthConfig.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
}

void LevelClient::getLevel(int id, Callback callback) {
	web::WebRequest req;
	auto token = gdvn::auth_config::getToken();

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

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}
