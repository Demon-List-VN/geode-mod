#include "PvpSubmitter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> PvpSubmitter::m_get_holder, PvpSubmitter::m_put_holder;

PvpSubmitter::PvpSubmitter() : m_state(std::make_shared<State>()) {}

PvpSubmitter::~PvpSubmitter() = default;

PvpSubmitter::PvpSubmitter(int levelID) : m_state(std::make_shared<State>(levelID)) {
	if (!AuthService::isLoggedIn()) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inPvp";
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	std::weak_ptr<State> state = m_state;
	m_get_holder.spawn(req.get(url), [state](web::WebResponse res) {
		try {
			if (!res.ok()) {
				return;
			}

			auto json = res.json().unwrap();

			if (json["matchId"].isNumber()) {
				if (auto locked = state.lock()) {
					locked->matchID = static_cast<int>(json["matchId"].asDouble().unwrap());
					locked->inPvp.store(locked->matchID > 0);
				}
			}
		} catch (...) {
			if (auto locked = state.lock()) {
				log::warn("Failed to check active PvP match for level {}", locked->levelID);
			} else {
				log::warn("Failed to check active PvP match");
			}
		}
	});
}

void PvpSubmitter::submit() {
	if (!m_state || !m_state->inPvp.load() || m_state->matchID <= 0) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/pvp/matches/" + std::to_string(m_state->matchID) + "/progress?progress=" + std::to_string(m_state->best);
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	m_put_holder.spawn(req.put(url), [](web::WebResponse res) {});
}

void PvpSubmitter::record(float progress) {
	if (!m_state || progress <= m_state->best) {
		return;
	}

	m_state->best = progress;
	submit();
}
