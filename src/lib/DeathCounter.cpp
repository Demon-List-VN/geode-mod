#include "DeathCounter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "../common.hpp"

DeathCounter::DeathCounter() {}

DeathCounter::DeathCounter(int id, bool completed) {
	deathData = DeathData(id, completed, {});
}

void DeathCounter::add(int percent) {
	deathData.addDeathCount(percent);
}

void DeathCounter::submit(geode::prelude::EventListener<geode::prelude::web::WebTask>* m_listener) {
	if (deathData.completed) {
		return;
	}

	using namespace geode::prelude;

	auto APIKey = geode::prelude::Mod::get()->getSettingValue<std::string>("API key");
	std::string urlPath = "/deathCount/" + std::to_string(deathData.levelID) + "/" + deathData.serialize();

	log::debug("{}", "POST " + API_URL + urlPath);

	web::WebRequest req = web::WebRequest();
	req.header("Authorization", "Bearer " + APIKey);
	m_listener->setFilter(req.post(API_URL + urlPath));

	std::cout << "DLVN: Sending attempt...";
}