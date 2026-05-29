#include "PvpClient.hpp"

#include "../../common.hpp"
#include "../../utils/AuthConfig.hpp"
#include <vector>

namespace {
async::TaskHolder<web::WebResponse> s_putHolder;
async::TaskHolder<web::WebResponse> s_postHolder;
async::TaskHolder<web::WebResponse> s_getHolder;
}

void PvpClient::putPlayMode(int matchID, std::string const& playMode, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/play-mode?playMode=" + playMode;

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_putHolder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::putProgress(int matchID, float progress, bool completed, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/progress?progress=" + std::to_string(progress);

	if (completed) {
		url += "&completed=true";
	}

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_putHolder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::postDeathCount(int matchID, std::string const& count, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/deaths?count=" + count;

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_postHolder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::getMessages(int matchID, std::int64_t afterID, int limit, Callback callback) {
	web::WebRequest req;
	auto url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/messages";
	std::vector<std::string> params;

	if (afterID > 0) {
		params.push_back("afterId=" + std::to_string(afterID));
	}

	if (limit > 0) {
		params.push_back("limit=" + std::to_string(limit));
	}

	if (!params.empty()) {
		url += "?";
		for (size_t i = 0; i < params.size(); ++i) {
			if (i > 0) {
				url += "&";
			}
			url += params[i];
		}
	}

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::postMessage(int matchID, std::string const& content, Callback callback) {
	web::WebRequest req;
	auto body = matjson::Value::object();
	body["content"] = content;
	req.bodyJSON(body);
	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	auto url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/messages";

	s_postHolder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}
