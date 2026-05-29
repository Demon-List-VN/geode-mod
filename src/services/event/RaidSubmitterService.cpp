#include "RaidSubmitterService.hpp"
#include <Geode/Geode.hpp>

#include "../auth/AuthService.hpp"
#include "../../clients/event/EventClient.hpp"

RaidSubmitterService::RaidSubmitterService() : m_state(std::make_shared<State>()) {}

RaidSubmitterService::~RaidSubmitterService() = default;

RaidSubmitterService::RaidSubmitterService(int levelID) : m_state(std::make_shared<State>(levelID)) {
	std::weak_ptr<State> state = m_state;
	EventClient::getEventLevel(levelID, "raid", [&](EmptyResponseDto const&, web::WebResponse& res) {
		if (auto locked = state.lock()) {
			locked->inEvent.store(res.ok());
		}
	});
}

void RaidSubmitterService::submit() {
	if (!m_state || !m_state->inEvent.load()) {
		return;
	}

	EventClient::putLevel(m_state->levelID, m_state->best, [&](EmptyResponseDto const&, web::WebResponse& res) {});
}

void RaidSubmitterService::record(float progress) {
	if (!m_state) {
		return;
	}

	m_state->best = progress;

	submit();
}
