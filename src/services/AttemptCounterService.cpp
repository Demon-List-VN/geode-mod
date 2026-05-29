#include "AttemptCounterService.hpp"
#include <Geode/Geode.hpp>

#include "AuthService.hpp"
#include "../clients/ProgressClient.hpp"

void AttemptCounterService::add() {
	cnt++;
}

void AttemptCounterService::submit() {
	using namespace geode::prelude;

	auto APIKey = AuthService::getToken();

	ProgressClient::submitHeatmap(cnt, APIKey, [&](web::WebResponse& res) {});
}
