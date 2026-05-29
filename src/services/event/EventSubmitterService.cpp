#include "EventSubmitterService.hpp"
#include <Geode/Geode.hpp>

#include "../auth/AuthService.hpp"
#include "../../clients/event/EventClient.hpp"

EventSubmitterService::EventSubmitterService() : m_state(std::make_shared<State>()) {}

EventSubmitterService::~EventSubmitterService() = default;

EventSubmitterService::EventSubmitterService(int levelID) : m_state(std::make_shared<State>(levelID)) {
	std::weak_ptr<State> state = m_state;
	EventClient::getEventLevel(levelID, "", [&](EmptyResponseDto const&, web::WebResponse& res) {
		if (auto locked = state.lock()) {
			locked->inEvent.store(res.ok());
		}
	});
}

void EventSubmitterService::submit() {
	if (!m_state || !m_state->inEvent.load()) {
		return;
	}

	EventClient::putLevel(m_state->levelID, m_state->best, [&](EmptyResponseDto const&, web::WebResponse& res) {});
}

void EventSubmitterService::record(float progress) {
	if (!m_state || progress <= m_state->best) {
		return;
	}

	m_state->best = progress;

	submit();
}
