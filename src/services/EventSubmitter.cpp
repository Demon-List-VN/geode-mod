#include "EventSubmitter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> EventSubmitter::m_get_holder, EventSubmitter::m_put_holder;

EventSubmitter::EventSubmitter() {}

EventSubmitter::~EventSubmitter() {
	m_get_holder.cancel();
	m_put_holder.cancel();
}

EventSubmitter::EventSubmitter(int levelID): levelID(levelID) {
	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent";
	std::string APIKey = AuthService::getToken();

	log::debug("[GDVN] EventSubmitter::EventSubmitter levelID={} checkUrl={}", levelID, url);

	req.header("Authorization", "Bearer " + APIKey);
	m_get_holder.spawn(req.get(url), [this, levelID](web::WebResponse const& res) {
		inEvent.store(res.ok());
		log::debug("[GDVN] EventSubmitter::EventSubmitter levelID={} inEvent={}", levelID, res.ok());
	});
}

void EventSubmitter::submit() {
	if (!inEvent.load()) {
		log::debug("[GDVN] EventSubmitter::submit skipped levelID={} progress={} reason=not-in-event", levelID, best);
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(best) + "&password=" + EVENT_PASSWORD;
	std::string APIKey = AuthService::getToken();

	log::debug("[GDVN] EventSubmitter::submit levelID={} progress={} url={}", levelID, best, url);

	req.header("Authorization", "Bearer " + APIKey);
	m_put_holder.spawn(req.put(url), [levelID = levelID, progress = best](web::WebResponse const& res) {
		log::debug("[GDVN] EventSubmitter::submit completed levelID={} progress={} ok={}", levelID, progress, res.ok());
	});
}

void EventSubmitter::record(float progress) {
	if (progress <= best) {
		log::debug("[GDVN] EventSubmitter::record skipped levelID={} progress={} best={} reason=not-improved", levelID, progress, best);
		return;
	}

	best = progress;
	log::debug("[GDVN] EventSubmitter::record accepted levelID={} progress={}", levelID, best);

	submit();
}