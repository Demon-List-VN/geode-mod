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

	log::debug("[GDVN] RaidSubmitter::RaidSubmitter levelID={} checkUrl={}", levelID, url);

	req.header("Authorization", "Bearer " + APIKey);
	m_get_holder.spawn(req.get(url), [this, levelID](web::WebResponse const& res) {
		inEvent.store(res.ok());
		log::debug("[GDVN] RaidSubmitter::RaidSubmitter levelID={} inEvent={}", levelID, res.ok());
	});
}

void RaidSubmitter::submit() {
	if (!inEvent.load()) {
		log::debug("[GDVN] RaidSubmitter::submit skipped levelID={} progress={} reason=not-in-event", levelID, best);
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(best) + "&password=" + EVENT_PASSWORD;
	std::string APIKey = AuthService::getToken();

	log::debug("[GDVN] RaidSubmitter::submit levelID={} progress={} url={}", levelID, best, url);

	req.header("Authorization", "Bearer " + APIKey);
	m_put_holder.spawn(req.put(url), [levelID = levelID, progress = best](web::WebResponse const& res) {
		log::debug("[GDVN] RaidSubmitter::submit completed levelID={} progress={} ok={}", levelID, progress, res.ok());
	});
}

void RaidSubmitter::record(float progress) {
	log::debug("[GDVN] RaidSubmitter::record levelID={} progress={} previousBest={}", levelID, progress, best);
	best = progress;

	submit();
}