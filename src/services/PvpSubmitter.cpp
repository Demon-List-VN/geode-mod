#include "PvpSubmitter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <algorithm>
#include <cmath>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> PvpSubmitter::m_get_holder, PvpSubmitter::m_put_holder, PvpSubmitter::m_death_holder, PvpSubmitter::m_mode_holder;

PvpSubmitter::PvpSubmitter() : m_state(std::make_shared<State>()) {}

PvpSubmitter::~PvpSubmitter() = default;

PvpSubmitter::PvpSubmitter(int levelID, std::string playMode) : m_state(std::make_shared<State>(levelID)) {
	m_state->playMode = playMode == "practice" ? "practice" : "normal";

	if (!AuthService::isLoggedIn()) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inPvp";
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	std::weak_ptr<State> state = m_state;
	m_get_holder.spawn(req.get(url), [state](web::WebResponse res) {
		if (!res.ok()) {
			return;
		}

		auto jsonResult = res.json();
		if (!jsonResult) {
			if (auto locked = state.lock()) {
				log::warn("Failed to parse active Versus match for level {}", locked->levelID);
			} else {
				log::warn("Failed to parse active Versus match");
			}
			return;
		}

		auto json = jsonResult.unwrap();
		if (json["matchId"].isNumber()) {
			if (auto locked = state.lock()) {
				locked->matchID = static_cast<int>(json["matchId"].asDouble().unwrapOr(0.0));
				if (json["mode"].isString()) {
					locked->platformer = json["mode"].asString().unwrapOrDefault() == "platformer";
				}
				locked->inPvp.store(locked->matchID > 0);
				if (locked->inPvp.load()) {
					PvpSubmitter::submitPlayMode(locked, locked->playMode);
				}
			}
		}
	});
}

void PvpSubmitter::submitPlayMode(std::shared_ptr<State> state, std::string const& playMode) {
	if (!state || !state->inPvp.load() || state->matchID <= 0) {
		return;
	}

	auto normalized = playMode == "practice" ? std::string("practice") : std::string("normal");
	if (state->submittedPlayMode == normalized) {
		return;
	}

	state->submittedPlayMode = normalized;

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/pvp/matches/" + std::to_string(state->matchID) + "/play-mode?playMode=" + normalized;
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	m_mode_holder.spawn(req.put(url), [normalized](web::WebResponse res) {
		if (!res.ok()) {
			log::warn("Failed to submit Versus play mode '{}': HTTP {}", normalized, res.code());
		}
	});
}

void PvpSubmitter::submit(bool completed) {
	if (!m_state || !m_state->inPvp.load() || m_state->matchID <= 0) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/pvp/matches/" + std::to_string(m_state->matchID) + "/progress?progress=" + std::to_string(m_state->best);
	if (completed) {
		url += "&completed=true";
	}
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	m_put_holder.spawn(req.put(url), [](web::WebResponse res) {});
}

void PvpSubmitter::submitDeathCount(std::shared_ptr<State> state) {
	if (!state || !state->inPvp.load() || state->platformer || state->matchID <= 0) {
		return;
	}

	const auto count = state->pendingDeathCount;
	if (sumDeathCount(count) <= 0) {
		return;
	}

	bool expected = false;
	if (!state->deathSubmitInFlight.compare_exchange_strong(expected, true)) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/pvp/matches/" + std::to_string(state->matchID) + "/deaths?count=" + serializeDeathCount(count);
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	std::weak_ptr<State> weakState = state;
	m_death_holder.spawn(req.post(url), [weakState, count](web::WebResponse res) {
		if (auto locked = weakState.lock()) {
			if (res.ok()) {
				for (size_t i = 0; i < locked->pendingDeathCount.size(); i++) {
					locked->pendingDeathCount[i] -= std::min(locked->pendingDeathCount[i], count[i]);
				}
			}
			locked->deathSubmitInFlight.store(false);

			if (res.ok() && PvpSubmitter::sumDeathCount(locked->pendingDeathCount) >= 100) {
				PvpSubmitter::submitDeathCount(locked);
			}
		}
	});
}

std::string PvpSubmitter::serializeDeathCount(std::array<size_t, 100> const& count) {
	std::string res;

	for (size_t value : count) {
		res += std::to_string(value) + "|";
	}

	if (!res.empty()) {
		res.pop_back();
	}

	return res;
}

size_t PvpSubmitter::sumDeathCount(std::array<size_t, 100> const& count) {
	size_t total = 0;
	for (size_t value : count) {
		total += value;
	}
	return total;
}

bool PvpSubmitter::isPlatformerPvp() const {
	return m_state && m_state->inPvp.load() && m_state->platformer;
}

void PvpSubmitter::submitPlayMode(std::string const& playMode) {
	if (!m_state) {
		return;
	}

	m_state->playMode = playMode == "practice" ? "practice" : "normal";
	submitPlayMode(m_state, m_state->playMode);
}

void PvpSubmitter::record(float progress) {
	if (!m_state || m_state->platformer || progress <= m_state->best) {
		return;
	}

	m_state->best = progress;
	submit();
}

void PvpSubmitter::recordDeath(float progress) {
	if (
		!m_state ||
		!m_state->inPvp.load() ||
		m_state->platformer ||
		!std::isfinite(progress) ||
		progress < 0.0f ||
		progress > 100.0f
	) {
		return;
	}

	const int percent = std::clamp(static_cast<int>(progress), 0, 99);
	m_state->pendingDeathCount[percent]++;
	if (progress > m_state->best) {
		m_state->best = progress;
		submit();
		flushDeathCount();
		return;
	}

	if (sumDeathCount(m_state->pendingDeathCount) >= 100) {
		flushDeathCount();
	}
}

void PvpSubmitter::flushDeathCount() {
	submitDeathCount(m_state);
}

void PvpSubmitter::recordCheckpoint(int count) {
	if (!m_state || !m_state->platformer || count <= m_state->best) {
		return;
	}

	m_state->best = static_cast<float>(count);
	submit();
}

void PvpSubmitter::completePlatformer(int count) {
	if (!m_state || !m_state->platformer) {
		return;
	}

	if (count > m_state->best) {
		m_state->best = static_cast<float>(count);
	}
	submit(true);
}
