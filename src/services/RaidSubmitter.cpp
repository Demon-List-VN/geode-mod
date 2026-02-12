#include "RaidSubmitter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> RaidSubmitter::m_get_holder, RaidSubmitter::m_put_holder;

RaidSubmitter::RaidSubmitter() {}

RaidSubmitter::~RaidSubmitter() {
	m_get_holder.cancel();
	m_put_holder.cancel();
}

RaidSubmitter::RaidSubmitter(int levelID): levelID(levelID) {
	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent?type=raid";
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	m_get_holder.spawn(req.get(url), [this](web::WebResponse res) {
		inEvent.store(res.ok());
	});
}

void RaidSubmitter::submit() {
	if (!inEvent.load()) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(best) + "&password=" + EVENT_PASSWORD;
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	m_put_holder.spawn(req.put(url), [](web::WebResponse res) {});
}

void RaidSubmitter::record(float progress) {
	best = progress;

	submit();
}