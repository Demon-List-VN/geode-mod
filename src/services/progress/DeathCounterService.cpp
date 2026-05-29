#include "DeathCounterService.hpp"
#include <Geode/Geode.hpp>

#include "../../clients/pvp/PvpProgressClient.hpp"
#include "../auth/AuthService.hpp"

DeathCounterService::DeathCounterService() {
}

DeathCounterService::DeathCounterService(int id, bool completed) {
    deathData = DeathData(id, completed, {});
}

void DeathCounterService::add(int percent) {
    deathData.addDeathCount(percent);
}

void DeathCounterService::submit() {
    using namespace geode::prelude;

    PvpProgressClient::postDeathCount(deathData.levelID, deathData.serialize(), completed,
                                      [=](EmptyResponseDto const&, web::WebResponse& res) {});
}
