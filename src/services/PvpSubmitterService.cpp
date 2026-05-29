#include "PvpSubmitterService.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>

#include "AuthService.hpp"
#include "../clients/LevelClient.hpp"
#include "../clients/PvpClient.hpp"
#include "../models/PvpModels.hpp"

PvpSubmitterService::PvpSubmitterService() : m_state(std::make_shared<State>()) {}

PvpSubmitterService::~PvpSubmitterService() = default;

PvpSubmitterService::PvpSubmitterService(int levelID, std::string playMode) : m_state(std::make_shared<State>(levelID)) {
	m_state->playMode = playMode == "practice" ? "practice" : "normal";

	if (!AuthService::isLoggedIn()) {
		return;
	}

	std::string APIKey = AuthService::getToken();

	std::weak_ptr<State> state = m_state;
	LevelClient::getActivePvpMatch(levelID, APIKey, [&](web::WebResponse& res) {
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

		auto match = gdvn::models::ActivePvpMatchResponseModel::fromJson(jsonResult.unwrap());
		if (match.valid) {
			if (auto locked = state.lock()) {
				locked->matchID = match.matchID;
				locked->platformer = match.mode == "platformer";
				locked->inPvp.store(locked->matchID > 0);
				if (locked->inPvp.load()) {
					PvpSubmitterService::submitPlayMode(locked, locked->playMode);
				}
			}
		}
	});
}

void PvpSubmitterService::submitPlayMode(std::shared_ptr<State> state, std::string const& playMode) {
	if (!state || !state->inPvp.load() || state->matchID <= 0) {
		return;
	}

	auto normalized = playMode == "practice" ? std::string("practice") : std::string("normal");
	if (state->submittedPlayMode == normalized) {
		return;
	}

	state->submittedPlayMode = normalized;

	std::string APIKey = AuthService::getToken();

	PvpClient::submitPlayMode(state->matchID, normalized, APIKey, [&](web::WebResponse& res) {
		if (!res.ok()) {
			log::warn("Failed to submit Versus play mode '{}': HTTP {}", normalized, res.code());
		}
	});
}

void PvpSubmitterService::submit(bool completed) {
	if (!m_state || !m_state->inPvp.load() || m_state->matchID <= 0) {
		return;
	}

	std::string APIKey = AuthService::getToken();

	PvpClient::submitProgress(m_state->matchID, m_state->best, completed, APIKey, [&](web::WebResponse& res) {});
}

void PvpSubmitterService::submitDeathCount(std::shared_ptr<State> state) {
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

	std::string APIKey = AuthService::getToken();

	std::weak_ptr<State> weakState = state;
	PvpClient::submitDeathCount(state->matchID, serializeDeathCount(count), APIKey, [&](web::WebResponse& res) {
		if (auto locked = weakState.lock()) {
			if (res.ok()) {
				for (size_t i = 0; i < locked->pendingDeathCount.size(); i++) {
					locked->pendingDeathCount[i] -= std::min(locked->pendingDeathCount[i], count[i]);
				}
			}
			locked->deathSubmitInFlight.store(false);

			if (res.ok() && PvpSubmitterService::sumDeathCount(locked->pendingDeathCount) >= 100) {
				PvpSubmitterService::submitDeathCount(locked);
			}
		}
	});
}

std::string PvpSubmitterService::serializeDeathCount(std::array<size_t, 100> const& count) {
	std::string res;

	for (size_t value : count) {
		res += std::to_string(value) + "|";
	}

	if (!res.empty()) {
		res.pop_back();
	}

	return res;
}

size_t PvpSubmitterService::sumDeathCount(std::array<size_t, 100> const& count) {
	size_t total = 0;
	for (size_t value : count) {
		total += value;
	}
	return total;
}

bool PvpSubmitterService::isPlatformerPvp() const {
	return m_state && m_state->inPvp.load() && m_state->platformer;
}

void PvpSubmitterService::submitPlayMode(std::string const& playMode) {
	if (!m_state) {
		return;
	}

	m_state->playMode = playMode == "practice" ? "practice" : "normal";
	submitPlayMode(m_state, m_state->playMode);
}

void PvpSubmitterService::record(float progress) {
	if (!m_state || m_state->platformer || progress <= m_state->best) {
		return;
	}

	m_state->best = progress;
	submit();
}

void PvpSubmitterService::recordDeath(float progress) {
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

void PvpSubmitterService::flushDeathCount() {
	submitDeathCount(m_state);
}

void PvpSubmitterService::recordCheckpoint(int count) {
	if (!m_state || !m_state->platformer || count <= m_state->best) {
		return;
	}

	m_state->best = static_cast<float>(count);
	submit();
}

void PvpSubmitterService::completePlatformer(int count) {
	if (!m_state || !m_state->platformer) {
		return;
	}

	if (count > m_state->best) {
		m_state->best = static_cast<float>(count);
	}
	submit(true);
}
