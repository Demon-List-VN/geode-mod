#include "DeathCounter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> DeathCounter::m_holder;

DeathCounter::DeathCounter() {}

DeathCounter::DeathCounter(int id, bool completed) {
	deathData = DeathData(id, completed, {});
}

void DeathCounter::add(int percent) {
	deathData.addDeathCount(percent);
}

void DeathCounter::submit() {
	if (deathData.completed) {
		return;
	}

	using namespace geode::prelude;

	auto APIKey = AuthService::getToken();
	std::string urlPath = "/deathCount/" + std::to_string(deathData.levelID) + "/" + deathData.serialize();

    if (completed) {
        urlPath += "?completed";
    }

	web::WebRequest req = web::WebRequest();
	req.header("Authorization", "Bearer " + APIKey);
	m_holder.spawn(req.post(API_URL + urlPath), [](web::WebResponse res) {});
}