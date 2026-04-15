#include "DeathCounter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> DeathCounter::m_holder;

DeathCounter::DeathCounter() {}

DeathCounter::DeathCounter(int id, bool completed) {
	deathData = DeathData(id, completed, {});
	this->completed = completed;
	log::debug("[GDVN] DeathCounter::DeathCounter levelID={} completed={}", id, completed);
}

void DeathCounter::add(int percent) {
	deathData.addDeathCount(percent);
	log::debug("[GDVN] DeathCounter::add levelID={} percent={} completed={}", deathData.levelID, percent, completed);
}

void DeathCounter::setCompleted(bool completed) {
	this->completed = completed;
	if (completed) {
		deathData.setCompleted();
	}
	log::debug("[GDVN] DeathCounter::setCompleted levelID={} completed={}", deathData.levelID, this->completed);
}

void DeathCounter::submit() {
	using namespace geode::prelude;

	auto APIKey = AuthService::getToken();
	std::string urlPath = "/deathCount/" + std::to_string(deathData.levelID) + "/" + deathData.serialize();

	if (completed) {
        urlPath += "?completed";
    }

	log::debug("[GDVN] DeathCounter::submit levelID={} completed={} url={}", deathData.levelID, completed, API_URL + urlPath);

	web::WebRequest req = web::WebRequest();
	req.header("Authorization", "Bearer " + APIKey);
	m_holder.spawn(req.post(API_URL + urlPath), [levelID = deathData.levelID](web::WebResponse const& res) {
		log::debug("[GDVN] DeathCounter::submit completed levelID={} ok={}", levelID, res.ok());
	});
}