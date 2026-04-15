#include "AttemptCounter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> AttemptCounter::m_holder;

void AttemptCounter::add() {
	cnt++;
	log::debug("[GDVN] AttemptCounter::add cnt={}", cnt);
}

void AttemptCounter::submit() {
	using namespace geode::prelude;

	auto APIKey = AuthService::getToken();
	std::string urlPath = "/players/heatmap/" + std::to_string(cnt);

	log::debug("[GDVN] AttemptCounter::submit cnt={} url={}", cnt, API_URL + urlPath);

	web::WebRequest req = web::WebRequest();
	req.header("Authorization", "Bearer " + APIKey);
	m_holder.spawn(req.post(API_URL + urlPath), [count = cnt](web::WebResponse const& res) {
		log::debug("[GDVN] AttemptCounter::submit completed cnt={} ok={}", count, res.ok());
	});
}