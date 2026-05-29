#include "LevelService.hpp"

#include "AuthService.hpp"
#include "../clients/LevelClient.hpp"

void LevelService::getLevel(int id, GetLevelCallback callback) {
	LevelClient::getLevel(id, AuthService::getToken(), [&](web::WebResponse& res) {
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
