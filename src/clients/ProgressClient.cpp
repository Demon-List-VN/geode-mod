#include "ProgressClient.hpp"

#include "../common.hpp"

async::TaskHolder<web::WebResponse> ProgressClient::m_post_holder;

void ProgressClient::submitHeatmap(size_t count, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/players/heatmap/" + std::to_string(count);

	req.header("Authorization", "Bearer " + token);

	m_post_holder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void ProgressClient::submitDeathCount(int levelID, std::string const& count, bool completed, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/deathCount/" + std::to_string(levelID) + "/" + count;

	if (completed) {
		url += "?completed";
	}

	req.header("Authorization", "Bearer " + token);

	m_post_holder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}
