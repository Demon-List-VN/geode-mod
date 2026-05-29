#include "DeathCounterService.hpp"
#include <Geode/Geode.hpp>

#include "AuthService.hpp"
#include "../clients/ProgressClient.hpp"

DeathCounterService::DeathCounterService() {}

DeathCounterService::DeathCounterService(int id, bool completed) {
	deathData = DeathData(id, completed, {});
}

void DeathCounterService::add(int percent) {
	deathData.addDeathCount(percent);
}

void DeathCounterService::submit() {
	using namespace geode::prelude;

	auto APIKey = AuthService::getToken();

	ProgressClient::submitDeathCount(deathData.levelID, deathData.serialize(), completed, APIKey, [&](web::WebResponse& res) {});
}
