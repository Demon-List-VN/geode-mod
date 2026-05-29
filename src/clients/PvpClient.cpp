#include "PvpClient.hpp"

#include "../common.hpp"
#include <vector>

async::TaskHolder<web::WebResponse> PvpClient::m_put_holder;
async::TaskHolder<web::WebResponse> PvpClient::m_post_holder;
async::TaskHolder<web::WebResponse> PvpClient::m_get_holder;

void PvpClient::submitPlayMode(int matchID, std::string const& playMode, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/play-mode?playMode=" + playMode;

	req.header("Authorization", "Bearer " + token);

	m_put_holder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::submitProgress(int matchID, float progress, bool completed, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/progress?progress=" + std::to_string(progress);

	if (completed) {
		url += "&completed=true";
	}

	req.header("Authorization", "Bearer " + token);

	m_put_holder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::submitDeathCount(int matchID, std::string const& count, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/deaths?count=" + count;

	req.header("Authorization", "Bearer " + token);

	m_post_holder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::getMessages(int matchID, std::int64_t afterID, int limit, std::string const& token, Callback callback) {
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

	req.header("Authorization", "Bearer " + token);

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void PvpClient::sendMessage(int matchID, std::string const& content, std::string const& token, Callback callback) {
	web::WebRequest req;
	auto body = matjson::Value::object();
	body["content"] = content;
	req.bodyJSON(body);
	req.header("Authorization", "Bearer " + token);

	auto url = API_URL + "/pvp/matches/" + std::to_string(matchID) + "/messages";

	m_post_holder.spawn(req.post(url), [&](web::WebResponse res) {
		callback(res);
	});
}
