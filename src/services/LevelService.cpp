#include "LevelService.hpp"

#include "../adapters/LevelInfoResponseAdapter.hpp"
#include "../clients/LevelClient.hpp"

void LevelService::getLevel(int id, GetLevelCallback callback) {
	LevelClient::getLevel(id, [&](web::WebResponse& res) {
		LevelInfoResponseDto level;

		if (res.ok()) {
			auto jsonResult = res.json();

			if (jsonResult) {
				level = gdvn::adapters::LevelInfoResponseAdapter::fromJson(jsonResult.unwrap());
			}
		}

		if (callback) {
			callback(level, res);
		}
	});
}
