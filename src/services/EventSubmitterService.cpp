#include "EventSubmitterService.hpp"
#include <Geode/Geode.hpp>

#include "AuthService.hpp"
#include "../clients/EventClient.hpp"
#include "../clients/LevelClient.hpp"

EventSubmitterService::EventSubmitterService() : m_state(std::make_shared<State>()) {}

EventSubmitterService::~EventSubmitterService() = default;

EventSubmitterService::EventSubmitterService(int levelID) : m_state(std::make_shared<State>(levelID)) {
	std::string APIKey = AuthService::getToken();

	std::weak_ptr<State> state = m_state;
	LevelClient::getEventLevel(levelID, APIKey, "", [&](web::WebResponse& res) {
		if (auto locked = state.lock()) {
			locked->inEvent.store(res.ok());
		}
	});
}

void EventSubmitterService::submit() {
	if (!m_state || !m_state->inEvent.load()) {
		return;
	}

	std::string APIKey = AuthService::getToken();

	EventClient::submitLevel(m_state->levelID, m_state->best, APIKey, [&](web::WebResponse& res) {});
}

void EventSubmitterService::record(float progress) {
	if (!m_state || progress <= m_state->best) {
		return;
	}

	m_state->best = progress;

	submit();
}
