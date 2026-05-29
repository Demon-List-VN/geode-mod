#include "LevelClient.hpp"

#include "../common.hpp"

async::TaskHolder<web::WebResponse> LevelClient::m_get_holder;

void LevelClient::getLevel(int id, std::string const& token, Callback callback) {
	web::WebRequest req;

	if (!token.empty()) {
		req.header("Authorization", "Bearer " + token);
	}

	auto url = API_URL + "/lists/levels/" + std::to_string(id) + "/starred";

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void LevelClient::getActivePvpMatch(int levelID, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inPvp";

	req.header("Authorization", "Bearer " + token);

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void LevelClient::getEventLevel(int levelID, std::string const& token, std::string const& type, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent";

	if (!type.empty()) {
		url += "?type=" + type;
	}

	req.header("Authorization", "Bearer " + token);

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}
