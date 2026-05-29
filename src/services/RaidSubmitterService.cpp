#include "RaidSubmitterService.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> RaidSubmitterService::m_get_holder, RaidSubmitterService::m_put_holder;

RaidSubmitterService::RaidSubmitterService() : m_state(std::make_shared<State>()) {}

RaidSubmitterService::~RaidSubmitterService() = default;

RaidSubmitterService::RaidSubmitterService(int levelID) : m_state(std::make_shared<State>(levelID)) {
	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent?type=raid";
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	std::weak_ptr<State> state = m_state;
	m_get_holder.spawn(req.get(url), [state](web::WebResponse res) {
		if (auto locked = state.lock()) {
			locked->inEvent.store(res.ok());
		}
	});
}

void RaidSubmitterService::submit() {
	if (!m_state || !m_state->inEvent.load()) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(m_state->levelID) + "?progress=" + std::to_string(m_state->best) + "&password=" + EVENT_PASSWORD;
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	m_put_holder.spawn(req.put(url), [](web::WebResponse res) {});
}

void RaidSubmitterService::record(float progress) {
	if (!m_state) {
		return;
	}

	m_state->best = progress;

	submit();
}
