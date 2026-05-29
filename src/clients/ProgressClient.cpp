#include "ProgressClient.hpp"

#include "../common.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_postHolder;
}

static std::string getToken() {
	return Mod::get()->getSavedValue<std::string>("api-key");
}

void ProgressClient::postHeatmap(size_t count, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/players/heatmap/" + std::to_string(count);

	req.header("Authorization", "Bearer " + getToken());

	s_postHolder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void ProgressClient::postDeathCount(int levelID, std::string const& count, bool completed, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/deathCount/" + std::to_string(levelID) + "/" + count;

	if (completed) {
		url += "?completed";
	}

	req.header("Authorization", "Bearer " + getToken());

	s_postHolder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}
