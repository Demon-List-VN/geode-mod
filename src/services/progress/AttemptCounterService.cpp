#include "AttemptCounterService.hpp"
#include <Geode/Geode.hpp>

#include "../../clients/pvp/PvpProgressClient.hpp"
#include "../auth/AuthService.hpp"

void AttemptCounterService::add() {
    cnt++;
}

void AttemptCounterService::submit() {
    using namespace geode::prelude;

    PvpProgressClient::postHeatmap(cnt, [=](EmptyResponseDto const&, web::WebResponse& res) {});
}
