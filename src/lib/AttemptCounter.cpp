#include "AttemptCounter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "../common.hpp"

void AttemptCounter::add() {
	cnt++;
}

void AttemptCounter::submit(geode::async::TaskHolder<geode::utils::web::WebResponse>* holder) {
	using namespace geode::prelude;

	auto APIKey = Mod::get()->getSettingValue<std::string>("API key");
	std::string urlPath = "/players/heatmap/" + std::to_string(cnt);

	log::debug("{}", "POST " + API_URL + urlPath);

	web::WebRequest req = web::WebRequest();
	req.header("Authorization", "Bearer " + APIKey);
	holder->spawn(req.post(API_URL + urlPath), [](web::WebResponse res) {});
}