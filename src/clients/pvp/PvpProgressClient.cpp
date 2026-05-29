#include "PvpProgressClient.hpp"

#include "../../common.hpp"
#include "../../utils/AuthConfig.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_postHolder;
}

void PvpProgressClient::postHeatmap(size_t count, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/players/heatmap/" + std::to_string(count);

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_postHolder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpProgressClient::postDeathCount(int levelID, std::string const& count, bool completed, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/deathCount/" + std::to_string(levelID) + "/" + count;

	if (completed) {
		url += "?completed";
	}

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_postHolder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}
