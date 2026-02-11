#include "RaidSubmitter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "../common.hpp"

RaidSubmitter::RaidSubmitter() {}

RaidSubmitter::~RaidSubmitter() {
	getHolder.cancel();
	putHolder.cancel();
}

RaidSubmitter::RaidSubmitter(int levelID): levelID(levelID) {
	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent?type=raid";
	std::string APIKey = geode::prelude::Mod::get()->getSettingValue<std::string>("API key");

	req.header("Authorization", "Bearer " + APIKey);
	getHolder.spawn(req.get(url), [this](web::WebResponse res) {
		inEvent.store(res.ok());
	});
}

void RaidSubmitter::submit() {
	if (!inEvent.load()) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(best) + "&password=" + EVENT_PASSWORD;
	std::string APIKey = geode::prelude::Mod::get()->getSettingValue<std::string>("API key");

	req.header("Authorization", "Bearer " + APIKey);
	putHolder.spawn(req.put(url), [](web::WebResponse res) {});
}

void RaidSubmitter::record(float progress) {
	best = progress;

	submit();
}