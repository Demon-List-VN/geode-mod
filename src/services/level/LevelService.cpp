#include "LevelService.hpp"

#include "../../clients/level/LevelClient.hpp"

void LevelService::getLevel(int id, GetLevelCallback callback) {
	LevelClient::getLevel(id, [=](LevelInfoResponseDto const& level, web::WebResponse& res) {
		if (callback) {
			callback(level, res);
		}
	});
}
