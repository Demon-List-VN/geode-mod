#include "LevelService.hpp"

#include "AuthService.hpp"
#include "../common.hpp"
#include <utility>

async::TaskHolder<web::WebResponse> LevelService::m_get_holder;

void LevelService::getLevel(int id, GetLevelCallback callback) {
	web::WebRequest req;

	if (AuthService::isLoggedIn()) {
		req.header("Authorization", "Bearer " + AuthService::getToken());
	}

	auto url = API_URL + "/lists/levels/" + std::to_string(id) + "/starred";

	m_get_holder.spawn(req.get(url), [&](web::WebResponse res) mutable {
		gdvn::models::LevelInfoResponseModel level;

		if (res.ok()) {
			auto jsonResult = res.json();

			if (jsonResult) {
				level = gdvn::models::LevelInfoResponseModel::fromJson(jsonResult.unwrap());
			}
		}

		if (callback) {
			callback(level, res);
		}
	});
}
