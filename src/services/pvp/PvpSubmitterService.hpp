#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace geode::prelude;

class PvpSubmitterService {
    struct ScoreSubmission {
        float value = 0.0f;
        bool death = false;
    };

    struct DeathSubmission {
        float value = 0.0f;
        std::string attemptID;
    };

    struct State {
        int levelID = 0;
        int matchID = 0;
        int matchLevelID = 0;
        float best = 0;
        float pendingBest = 0;
        float progressRunSnapshot = 0;
        std::chrono::steady_clock::time_point progressRunSampleStartedAt;
        std::chrono::steady_clock::time_point progressRunWaitStartedAt;
        std::array<size_t, 100> pendingDeathCount = {};
        std::vector<DeathSubmission> pendingDeathProgresses;
        std::vector<ScoreSubmission> pendingScoreSubmissions;
        std::atomic<bool> deathSubmitInFlight{false};
        std::atomic<bool> scoreSubmitInFlight{false};
        bool platformer = false;
        bool completionPending = false;
        bool progressRunActive = false;
        bool progressRunSampling = false;
        bool progressRunWaiting = false;
        bool progressRunRequestInFlight = false;
        int progressRunLastRateCenti = -1;
        bool scoreMode = false;
        bool matchLookupCompleted = false;
        std::string playMode = "normal";
        std::string submittedPlayMode;
        std::string progressRunAttemptID;
        std::atomic<bool> inPvp{false};

        explicit State(int levelID = 0) : levelID(levelID) {
        }
    };

    std::shared_ptr<State> m_state;

    void submit(bool completed = false);
    static void submitProgress(std::shared_ptr<State> state, bool completed = false);
    static void submitDeathCount(std::shared_ptr<State> state);
    static void submitDeathProgress(std::shared_ptr<State> state, float progress, std::string const& attemptID = "");
    static void announceProgressRun(std::shared_ptr<State> state, float progress, float progressSpeed,
                                    int rateCenti, std::string const& attemptID);
    static void submitScoreSubmission(std::shared_ptr<State> state);
    static void submitPlayMode(std::shared_ptr<State> state, std::string const& playMode);
    static bool isLevelValid(std::shared_ptr<State> state);
    static std::string serializeDeathCount(std::array<size_t, 100> const& count);
    static size_t sumDeathCount(std::array<size_t, 100> const& count);
    static std::string createProgressRunAttemptID(int matchID);
    static void resetProgressRunState(std::shared_ptr<State> state);

  public:
    PvpSubmitterService();
    ~PvpSubmitterService();
    PvpSubmitterService(int levelID, std::string playMode = "normal");
    bool isPlatformerPvp() const;
    void setMatchLevelID(int levelID);
    void submitPlayMode(std::string const& playMode);
    void record(float progress);
    void recordRunProgress(float progress, float dt);
    void recordDeath(float progress);
    void setBestProgress(float progress);
    void flushDeathCount();
    void stopProgressRun();
    void resetProgressState();
    void recordCheckpoint(int count);
    void completePlatformer(int count);
};
