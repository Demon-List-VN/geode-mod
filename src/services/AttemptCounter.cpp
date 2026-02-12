#include "AttemptCounter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> AttemptCounter::m_holder;

void AttemptCounter::add() {
	cnt++;
}

void AttemptCounter::submit() {
	using namespace geode::prelude;

	auto APIKey = AuthService::getToken();
	std::string urlPath = "/players/heatmap/" + std::to_string(cnt);

	log::debug("{}", "POST " + API_URL + urlPath);

	web::WebRequest req = web::WebRequest();
	req.header("Authorization", "Bearer " + APIKey);
	m_holder.spawn(req.post(API_URL + urlPath), [](web::WebResponse res) {});
}