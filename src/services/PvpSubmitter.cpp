#include "PvpSubmitter.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/web.hpp>
#include <algorithm>
#include <cmath>

#include "AuthService.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> PvpSubmitter::m_get_holder, PvpSubmitter::m_put_holder, PvpSubmitter::m_death_holder;

namespace {
std::string getString(matjson::Value const& json, char const* key) {
	if (!json[key].isString()) {
		return "";
	}

	return json[key].asString().unwrapOrDefault();
}

float getNumber(matjson::Value const& json, char const* key) {
	if (!json[key].isNumber()) {
		return 0.0f;
	}

	return static_cast<float>(json[key].asDouble().unwrapOr(0.0));
}

float normalizeProgress(float progress, bool platformer) {
	if (!std::isfinite(progress)) {
		return 0.0f;
	}

	if (platformer) {
		return std::max(0.0f, std::floor(progress));
	}

	return std::clamp(progress, 0.0f, 100.0f);
}

float getResultProgressForUid(matjson::Value const& row, std::string const& uid, bool platformer) {
	if (uid.empty() || getString(row, "uid") != uid) {
		return 0.0f;
	}

	auto progress = getNumber(row, "progress");
	if (!row["result"].isNull() && row["result"].isObject()) {
		progress = std::max(progress, getNumber(row["result"], "progress"));
	}
	return normalizeProgress(progress, platformer);
}

float getApiProgress(matjson::Value const& json, std::string const& uid, bool platformer) {
	const auto currentUid = uid.empty() ? getString(json, "currentUid") : uid;
	float progress = 0.0f;

	if (json["results"].isArray()) {
		for (auto const& result : json["results"].asArray().unwrap()) {
			progress = std::max(progress, getResultProgressForUid(result, currentUid, platformer));
		}
	}

	if (json["participants"].isArray()) {
		for (auto const& participant : json["participants"].asArray().unwrap()) {
			progress = std::max(progress, getResultProgressForUid(participant, currentUid, platformer));
		}
	}

	return progress;
}
}

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
					if (json["mode"].isString()) {
						locked->platformer = json["mode"].asString().unwrapOrDefault() == "platformer";
					}
					locked->inPvp.store(locked->matchID > 0);
					locked->currentUid = getString(json, "currentUid");
					const float apiProgress = getApiProgress(json, locked->currentUid, locked->platformer);
					locked->apiBest = std::max(locked->apiBest, apiProgress);
					locked->submittedBest = std::max(locked->submittedBest, locked->apiBest);
					locked->sessionBest = std::max(locked->sessionBest, apiProgress);
					if (locked->inPvp.load() && locked->sessionBest > locked->submittedBest) {
						locked->submittedBest = locked->sessionBest;
						PvpSubmitter::submitProgress(
							locked,
							locked->sessionBest,
							false,
							locked->progressSubmitGeneration.fetch_add(1) + 1
						);
					}
				}
			}
		} catch (...) {
			if (auto locked = state.lock()) {
				log::warn("Failed to check active Versus match for level {}", locked->levelID);
			} else {
				log::warn("Failed to check active Versus match");
			}
		}
	});
}

void PvpSubmitter::submit(bool completed) {
	if (!m_state || !m_state->inPvp.load() || m_state->matchID <= 0) {
		return;
	}

	const float progress = m_state->sessionBest;
	if (!completed && (progress <= m_state->apiBest || progress <= m_state->submittedBest)) {
		return;
	}

	m_state->submittedBest = progress;
	const int generation = m_state->progressSubmitGeneration.fetch_add(1) + 1;
	m_state->progressRetryPending = false;
	submitProgress(m_state, progress, completed, generation);
}

void PvpSubmitter::submitProgress(
	std::shared_ptr<State> state,
	float progress,
	bool completed,
	int generation
) {
	if (
		!state ||
		!state->inPvp.load() ||
		state->matchID <= 0 ||
		state->progressSubmitGeneration.load() != generation
	) {
		return;
	}

	web::WebRequest req = web::WebRequest();
	std::string url = API_URL + "/pvp/matches/" + std::to_string(state->matchID) +
		"/progress?progress=" + std::to_string(progress);
	if (completed) {
		url += "&completed=true";
	}
	std::string APIKey = AuthService::getToken();

	req.header("Authorization", "Bearer " + APIKey);
	std::weak_ptr<State> weakState = state;
	m_put_holder.spawn(req.put(url), [weakState, progress, completed, generation](web::WebResponse res) {
		if (auto locked = weakState.lock()) {
			if (locked->progressSubmitGeneration.load() != generation) {
				return;
			}

			if (res.ok()) {
				try {
					auto json = res.json().unwrap();
					const float apiProgress = getApiProgress(json, locked->currentUid, locked->platformer);
					locked->apiBest = std::max(locked->apiBest, apiProgress);
					locked->submittedBest = std::max(locked->submittedBest, locked->apiBest);
					locked->sessionBest = std::max(locked->sessionBest, apiProgress);
				} catch (...) {
					log::warn("Failed to parse saved Versus progress response for match {}", locked->matchID);
				}
				return;
			}

			PvpSubmitter::scheduleProgressRetry(weakState, progress, completed, generation);
		}
	});
}

void PvpSubmitter::scheduleProgressRetry(
	std::weak_ptr<State> state,
	float progress,
	bool completed,
	int generation
) {
	geode::Loader::get()->queueInMainThread([state, progress, completed, generation] {
		geode::Notification::create(
			"Internet unstable. Retrying progress update in 5 seconds...",
			geode::NotificationIcon::Error,
			3.0f
		)->show();

		if (auto locked = state.lock()) {
			if (locked->progressSubmitGeneration.load() != generation) {
				return;
			}

			locked->progressRetryPending = true;
			locked->progressRetryDelay = 5.0f;
			locked->progressRetryValue = progress;
			locked->progressRetryCompleted = completed;
			locked->progressRetryGeneration = generation;
		}
	});
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

void PvpSubmitter::update(float dt) {
	if (!m_state || !m_state->progressRetryPending) {
		return;
	}

	m_state->progressRetryDelay -= dt;
	if (m_state->progressRetryDelay > 0.0f) {
		return;
	}

	const float progress = m_state->progressRetryValue;
	const bool completed = m_state->progressRetryCompleted;
	const int generation = m_state->progressRetryGeneration;
	m_state->progressRetryPending = false;
	submitProgress(m_state, progress, completed, generation);
}

bool PvpSubmitter::isPlatformerPvp() const {
	return m_state && m_state->inPvp.load() && m_state->platformer;
}

void PvpSubmitter::syncApiProgress(float progress) {
	if (!m_state || !std::isfinite(progress)) {
		return;
	}

	const float apiProgress = normalizeProgress(progress, m_state->platformer);
	m_state->apiBest = std::max(m_state->apiBest, apiProgress);
	m_state->submittedBest = std::max(m_state->submittedBest, m_state->apiBest);
	m_state->sessionBest = std::max(m_state->sessionBest, apiProgress);
}

void PvpSubmitter::record(float progress) {
	if (!m_state || m_state->platformer || !std::isfinite(progress) || progress <= m_state->sessionBest) {
		return;
	}

	m_state->sessionBest = progress;
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
	if (progress > m_state->sessionBest) {
		m_state->sessionBest = progress;
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
	if (!m_state || !m_state->platformer || count <= m_state->sessionBest) {
		return;
	}

	m_state->sessionBest = static_cast<float>(count);
	submit();
}

void PvpSubmitter::completePlatformer(int count) {
	if (!m_state || !m_state->platformer) {
		return;
	}

	if (count > m_state->sessionBest) {
		m_state->sessionBest = static_cast<float>(count);
	}
	submit(true);
}
