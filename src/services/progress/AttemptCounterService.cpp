#include "AttemptCounterService.hpp"
#include <Geode/Geode.hpp>

#include "../auth/AuthService.hpp"
#include "../../clients/pvp/PvpProgressClient.hpp"

void AttemptCounterService::add() {
	cnt++;
}

void AttemptCounterService::submit() {
	using namespace geode::prelude;

	PvpProgressClient::postHeatmap(cnt, [&](web::WebResponse& res) {});
}
