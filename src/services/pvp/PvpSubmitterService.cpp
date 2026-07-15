#include "PvpSubmitterService.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "../../clients/level/LevelClient.hpp"
#include "../../clients/pvp/PvpClient.hpp"
#include "../auth/AuthService.hpp"

namespace {
constexpr float PROGRESS_RUN_SAMPLE_SECONDS = 0.1f;
constexpr float PROGRESS_RUN_WAIT_SECONDS = 3.0f;
std::atomic<std::uint64_t> s_progressRunAttemptCounter{0};
}

PvpSubmitterService::PvpSubmitterService() : m_state(std::make_shared<State>()) {
}

PvpSubmitterService::~PvpSubmitterService() = default;

PvpSubmitterService::PvpSubmitterService(int levelID, std::string playMode)
    : m_state(std::make_shared<State>(levelID)) {
    m_state->playMode = playMode == "practice" ? "practice" : "normal";

    if (!AuthService::isLoggedIn()) {
        log::info("Skipping Versus match fetch for level {}: user is not logged in", levelID);
        return;
    }

    log::info("Fetching active Versus match for level {} (playMode={})", levelID, m_state->playMode);

    std::weak_ptr<State> state = m_state;
    LevelClient::getActivePvpMatch(levelID, [=](ActivePvpMatchResponseDto const& match, web::WebResponse& res) {
        auto applyMatch = [=](ActivePvpMatchResponseDto const& resolvedMatch) {
            if (auto locked = state.lock()) {
                if (locked->matchLevelID <= 0) {
                    locked->matchLevelID = resolvedMatch.levelID > 0 ? resolvedMatch.levelID : locked->levelID;
                }
                locked->matchID = resolvedMatch.matchID;
                locked->platformer = resolvedMatch.mode == "platformer";
                locked->scoreMode = resolvedMatch.scoringMode == "score" || resolvedMatch.scoringMode == "hp" ||
                    resolvedMatch.scoringMode == "powerup";
                locked->best = std::max(locked->best, resolvedMatch.bestProgress);
                locked->matchLookupCompleted = true;
                locked->inPvp.store(locked->matchID > 0);
                const bool levelValid = PvpSubmitterService::isLevelValid(locked);

                log::info(
                    "Active Versus match for level {}: matchID={}, matchLevelID={}, mode={}, scoringMode={}, targetScore={}, startingHp={}, finalizeAliveCount={}, status={}, context={}, roomName={}, levelValid={}, queuedScoreEvents={}",
                    locked->levelID, locked->matchID, locked->matchLevelID, resolvedMatch.mode,
                    resolvedMatch.scoringMode, resolvedMatch.targetScore, resolvedMatch.startingHp,
                    resolvedMatch.finalizeAliveCount, resolvedMatch.status, resolvedMatch.context, resolvedMatch.roomName,
                    levelValid, locked->pendingScoreSubmissions.size()
                );

                if (locked->scoreMode) {
                    log::info(
                        "Versus {} mode active for match {}: queued values are events, not best progress",
                        resolvedMatch.scoringMode, locked->matchID
                    );
                }

                if (locked->inPvp.load() && levelValid) {
                    PvpSubmitterService::submitPlayMode(locked, locked->playMode);
                    if (locked->scoreMode) {
                        locked->pendingDeathCount = {};
                        PvpSubmitterService::submitScoreSubmission(locked);
                    } else {
                        locked->pendingScoreSubmissions.clear();
                        for (auto const& submission : locked->pendingDeathProgresses) {
                            PvpSubmitterService::submitDeathProgress(
                                locked, submission.value, submission.attemptID
                            );
                        }
                        locked->pendingDeathProgresses.clear();
                        if (locked->completionPending) {
                            PvpSubmitterService::submitProgress(locked, true);
                        }
                    }
                }
            }
        };

        if (auto locked = state.lock()) {
            if (!res.ok()) {
                locked->matchLookupCompleted = true;
                locked->pendingScoreSubmissions.clear();

                if (res.code() == 404) {
                    log::info("No active Versus match found for level {} (HTTP 404)", locked->levelID);
                } else {
                    log::warn("Failed to fetch active Versus match for level {}: HTTP {}", locked->levelID, res.code());
                }
                return;
            }

            if (!match.valid) {
                locked->matchLookupCompleted = true;
                locked->pendingScoreSubmissions.clear();
                log::warn("Fetched active Versus match for level {}, but the response could not be mapped",
                          locked->levelID);
                return;
            }

            if (match.context == "custom_room" && match.matchID > 0) {
                PvpClient::getMatch(match.matchID, [=](PvpMatchSnapshotDto const& detail, web::WebResponse& detailRes) {
                    auto resolvedMatch = match;
                    if (detailRes.ok() && detail.matchID > 0) {
                        resolvedMatch.scoringMode = detail.scoringMode;
                        resolvedMatch.targetScore = detail.targetScore;
                        resolvedMatch.startingHp = detail.startingHp;
                        resolvedMatch.finalizeAliveCount = detail.finalizeAliveCount;
                        if (!detail.mode.empty()) {
                            resolvedMatch.mode = detail.mode;
                        }
                        log::info(
                            "Resolved custom room scoring metadata for match {}: scoringMode={}, targetScore={}, startingHp={}, finalizeAliveCount={}",
                            resolvedMatch.matchID, resolvedMatch.scoringMode, resolvedMatch.targetScore,
                            resolvedMatch.startingHp, resolvedMatch.finalizeAliveCount
                        );
                    } else if (!detailRes.ok()) {
                        log::warn("Failed to resolve custom room scoring metadata for match {}: HTTP {}",
                                  match.matchID, detailRes.code());
                    }

                    applyMatch(resolvedMatch);
                });
                return;
            }

            applyMatch(match);
        }
    });
}

void PvpSubmitterService::submitPlayMode(std::shared_ptr<State> state, std::string const& playMode) {
    if (!state || !state->inPvp.load() || state->matchID <= 0 || !isLevelValid(state)) {
        return;
    }

    auto normalized = playMode == "practice" ? std::string("practice") : std::string("normal");
    if (state->submittedPlayMode == normalized) {
        return;
    }

    state->submittedPlayMode = normalized;

    PvpClient::putPlayMode(state->matchID, normalized, [=](EmptyResponseDto const&, web::WebResponse& res) {
        if (!res.ok()) {
            log::warn("Failed to submit Versus play mode '{}': HTTP {}", normalized, res.code());
        }
    });
}

void PvpSubmitterService::submit(bool completed) {
    if (!m_state) {
        return;
    }

    m_state->completionPending = m_state->completionPending || completed;
    submitProgress(m_state, m_state->completionPending);
}

void PvpSubmitterService::submitProgress(std::shared_ptr<State> state, bool completed) {
    if (!state || !state->inPvp.load() || state->matchID <= 0 || state->best <= 0.0f || !isLevelValid(state)) {
        return;
    }

    const bool submitCompleted = completed || state->completionPending;
    const int matchID = state->matchID;
    const float progress = state->best;
    const std::string attemptID = submitCompleted ? state->progressRunAttemptID : "";

    PvpClient::putProgress(matchID, progress, submitCompleted,
                           [matchID, progress, submitCompleted](EmptyResponseDto const&, web::WebResponse& res) {
                               if (!res.ok()) {
                                   log::warn("Failed to submit Versus progress {} for match {}{}: HTTP {}", progress,
                                             matchID, submitCompleted ? " (completed)" : "", res.code());
                               }
                           }, attemptID);
}

void PvpSubmitterService::submitDeathCount(std::shared_ptr<State> state) {
    if (!state || !state->inPvp.load() || state->platformer || state->scoreMode || state->matchID <= 0
        || !isLevelValid(state)) {
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

    std::weak_ptr<State> weakState = state;
    PvpClient::postDeathCount(
        state->matchID, serializeDeathCount(count), [=](EmptyResponseDto const&, web::WebResponse& res) {
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

void PvpSubmitterService::submitDeathProgress(std::shared_ptr<State> state, float progress,
                                              std::string const& attemptID) {
    if (!state || !state->inPvp.load() || state->platformer || state->scoreMode || state->matchID <= 0
        || !isLevelValid(state)) {
        return;
    }

    const int matchID = state->matchID;
    std::weak_ptr<State> weakState = state;
    PvpClient::postDeathProgress(
        matchID, progress,
        [matchID, progress, weakState](EmptyResponseDto const&, web::WebResponse& res) {
            if (!res.ok()) {
                log::warn("Failed to submit Versus death progress {} for match {}: HTTP {}", progress, matchID,
                          res.code());
            }

            if (auto locked = weakState.lock()) {
                if (res.ok()) {
                    locked->best = std::max(locked->best, progress);
                }
                if (locked->pendingBest <= progress || locked->pendingBest <= locked->best) {
                    locked->pendingBest = locked->best;
                }
            }
        },
        attemptID
    );
}

void PvpSubmitterService::announceProgressRun(std::shared_ptr<State> state, float progress, float progressSpeed,
                                              int rateCenti, std::string const& attemptID) {
    if (!state || !state->inPvp.load() || state->platformer || state->scoreMode || state->matchID <= 0
        || !isLevelValid(state)) {
        return;
    }

    const int matchID = state->matchID;
    std::weak_ptr<State> weakState = state;
    PvpClient::postProgressRun(
        matchID, progress, progressSpeed, attemptID,
        [=](EmptyResponseDto const&, web::WebResponse& res) {
            if (!res.ok()) {
                log::warn("Failed to announce Versus progress run {} at {}/s for match {}: HTTP {}", progress,
                          progressSpeed, matchID, res.code());
            }

            if (auto locked = weakState.lock()) {
                if (!locked->progressRunActive || locked->progressRunAttemptID != attemptID) {
                    return;
                }

                locked->progressRunRequestInFlight = false;
                if (res.ok()) {
                    locked->progressRunLastRateCenti = rateCenti;
                }
                locked->progressRunWaiting = true;
                locked->progressRunWaitStartedAt = std::chrono::steady_clock::now();
            }
        }
    );
}

void PvpSubmitterService::submitScoreSubmission(std::shared_ptr<State> state) {
    if (!state || !state->inPvp.load() || state->platformer || !state->scoreMode || state->matchID <= 0
        || !isLevelValid(state) || state->pendingScoreSubmissions.empty()) {
        return;
    }

    bool expected = false;
    if (!state->scoreSubmitInFlight.compare_exchange_strong(expected, true)) {
        return;
    }

    const int matchID = state->matchID;
    const auto submission = state->pendingScoreSubmissions.front();
    const float input = submission.value;
    const bool completed = !submission.death && input >= 100.0f;
    std::weak_ptr<State> weakState = state;
    const auto kind = submission.death ? "death" : "run";

    log::info("Submitting Versus score {} for match {}: input={}, completed={}, queued={}", kind, matchID, input,
              completed, state->pendingScoreSubmissions.size());

    auto callback = [=](EmptyResponseDto const&, web::WebResponse& res) {
        if (auto locked = weakState.lock()) {
            if (res.ok()) {
                if (!locked->pendingScoreSubmissions.empty()) {
                    locked->pendingScoreSubmissions.erase(locked->pendingScoreSubmissions.begin());
                }
                if (submission.death) {
                    locked->pendingDeathCount = {};
                }
            } else {
                log::warn("Failed to submit Versus score {} for match {} with input {}: HTTP {}", kind, matchID,
                          input, res.code());
            }

            locked->scoreSubmitInFlight.store(false);

            if (res.ok()) {
                PvpSubmitterService::submitScoreSubmission(locked);
            }
        }
    };

    if (submission.death) {
        PvpClient::postDeathProgress(matchID, input, callback);
        return;
    }

    PvpClient::putProgress(matchID, input, completed, callback);
}

bool PvpSubmitterService::isLevelValid(std::shared_ptr<State> state) {
    return state && state->levelID > 0 && state->matchLevelID == state->levelID;
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

std::string PvpSubmitterService::createProgressRunAttemptID(int matchID) {
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()
    )
                         .count();
    const auto counter = s_progressRunAttemptCounter.fetch_add(1);

    return fmt::format("{}-{}-{}", matchID, now, counter);
}

void PvpSubmitterService::resetProgressRunState(std::shared_ptr<State> state) {
    if (!state) {
        return;
    }

    state->progressRunSnapshot = 0.0f;
    state->progressRunSampleStartedAt = {};
    state->progressRunWaitStartedAt = {};
    state->progressRunActive = false;
    state->progressRunSampling = false;
    state->progressRunWaiting = false;
    state->progressRunRequestInFlight = false;
    state->progressRunLastRateCenti = -1;
    state->progressRunAttemptID.clear();
}

bool PvpSubmitterService::isPlatformerPvp() const {
    return m_state && m_state->inPvp.load() && m_state->platformer;
}

void PvpSubmitterService::setMatchLevelID(int levelID) {
    if (!m_state) {
        return;
    }

    m_state->matchLevelID = levelID;
}

void PvpSubmitterService::submitPlayMode(std::string const& playMode) {
    if (!m_state) {
        return;
    }

    m_state->playMode = playMode == "practice" ? "practice" : "normal";
    submitPlayMode(m_state, m_state->playMode);
}

void PvpSubmitterService::record(float progress) {
    if (!m_state || m_state->platformer || !std::isfinite(progress) || progress <= 0.0f || progress > 100.0f) {
        return;
    }

    const float runProgress = std::min(progress, 100.0f);
    const bool completed = runProgress >= 100.0f;

    if (m_state->scoreMode) {
        m_state->pendingScoreSubmissions.push_back({runProgress, false});
        if (completed) {
            m_state->completionPending = true;
        }
        submitScoreSubmission(m_state);
        return;
    }

    if (!m_state->matchLookupCompleted) {
        m_state->pendingScoreSubmissions.push_back({runProgress, false});
    }

    if (runProgress <= m_state->best) {
        return;
    }

    m_state->best = runProgress;
    submit(completed);
    if (completed) {
        resetProgressRunState(m_state);
    }
}

void PvpSubmitterService::recordRunProgress(float progress, float dt) {
    if (!m_state || m_state->platformer || m_state->scoreMode || !std::isfinite(progress) || progress <= 0.0f
        || progress >= 100.0f || !std::isfinite(dt) || dt < 0.0f) {
        return;
    }

    if (!m_state->matchLookupCompleted || !m_state->inPvp.load() || !isLevelValid(m_state)) {
        return;
    }

    if (progress <= std::max(m_state->best, m_state->pendingBest)) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    if (!m_state->progressRunActive) {
        m_state->progressRunActive = true;
        m_state->progressRunSampling = true;
        m_state->progressRunWaiting = false;
        m_state->progressRunRequestInFlight = false;
        m_state->progressRunSnapshot = progress;
        m_state->progressRunSampleStartedAt = now;
        m_state->progressRunWaitStartedAt = {};
        m_state->progressRunLastRateCenti = -1;
        m_state->progressRunAttemptID = createProgressRunAttemptID(m_state->matchID);
        return;
    }

    if (m_state->progressRunRequestInFlight) {
        return;
    }

    if (m_state->progressRunWaiting) {
        const float waitSeconds = std::chrono::duration<float>(
            now - m_state->progressRunWaitStartedAt
        )
                                      .count();
        if (waitSeconds < PROGRESS_RUN_WAIT_SECONDS) {
            return;
        }

        m_state->progressRunWaiting = false;
        m_state->progressRunSampling = true;
        m_state->progressRunSnapshot = progress;
        m_state->progressRunSampleStartedAt = now;
        return;
    }

    if (!m_state->progressRunSampling) {
        return;
    }

    const float sampleSeconds = std::chrono::duration<float>(
        now - m_state->progressRunSampleStartedAt
    )
                                    .count();
    if (sampleSeconds < PROGRESS_RUN_SAMPLE_SECONDS) {
        return;
    }

    const float speed = std::max(0.0f, (progress - m_state->progressRunSnapshot) / sampleSeconds);
    const int rateCenti = static_cast<int>(std::lround(speed * 100.0f));
    m_state->progressRunSampling = false;

    if (rateCenti == m_state->progressRunLastRateCenti) {
        m_state->progressRunWaiting = true;
        m_state->progressRunWaitStartedAt = now;
        return;
    }

    m_state->progressRunRequestInFlight = true;
    announceProgressRun(
        m_state,
        progress,
        static_cast<float>(rateCenti) / 100.0f,
        rateCenti,
        m_state->progressRunAttemptID
    );
}

void PvpSubmitterService::recordDeath(float progress) {
    if (!m_state || m_state->platformer || !std::isfinite(progress) || progress < 0.0f || progress > 100.0f) {
        return;
    }

    const float deathProgress = std::min(progress, 99.99f);
    const std::string attemptID = m_state->progressRunAttemptID;

    if (!m_state->matchLookupCompleted || m_state->scoreMode) {
        m_state->pendingScoreSubmissions.push_back({deathProgress, true});
    }

    if (m_state->scoreMode) {
        if (m_state->inPvp.load()) {
            submitScoreSubmission(m_state);
        }
        return;
    }

    resetProgressRunState(m_state);
    m_state->pendingBest = std::max(m_state->pendingBest, deathProgress);

    if (!m_state->matchLookupCompleted) {
        m_state->pendingDeathProgresses.push_back({deathProgress, attemptID});
        return;
    }

    submitDeathProgress(m_state, deathProgress, attemptID);
}

void PvpSubmitterService::setBestProgress(float progress) {
    if (!m_state || !std::isfinite(progress) || progress < 0.0f) {
        return;
    }

    m_state->best = std::max(m_state->best, progress);
    if (m_state->pendingBest <= m_state->best) {
        m_state->pendingBest = m_state->best;
    }
}

void PvpSubmitterService::flushDeathCount() {
    if (m_state && m_state->scoreMode) {
        submitScoreSubmission(m_state);
        return;
    }

    submitDeathCount(m_state);
}

void PvpSubmitterService::stopProgressRun() {
    resetProgressRunState(m_state);
    if (m_state) {
        m_state->inPvp.store(false);
    }
}

void PvpSubmitterService::resetProgressState() {
    if (!m_state) {
        return;
    }

    m_state->best = 0.0f;
    m_state->pendingBest = 0.0f;
    resetProgressRunState(m_state);
    m_state->pendingDeathCount = {};
    m_state->pendingDeathProgresses.clear();
    m_state->pendingScoreSubmissions.clear();
    m_state->completionPending = false;
    m_state->deathSubmitInFlight.store(false);
    m_state->scoreSubmitInFlight.store(false);
}

void PvpSubmitterService::recordCheckpoint(int count) {
    if (!m_state || count <= m_state->best) {
        return;
    }

    m_state->best = static_cast<float>(count);
    submit();
}

void PvpSubmitterService::completePlatformer(int count) {
    if (!m_state) {
        return;
    }

    m_state->completionPending = true;
    if (count > m_state->best) {
        m_state->best = static_cast<float>(count);
    }
    submit(true);
}
