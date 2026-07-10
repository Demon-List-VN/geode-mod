#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

using namespace geode::prelude;

class PvpSubmitterService {
    struct ScoreSubmission {
        float value = 0.0f;
        bool death = false;
    };

    struct State {
        int levelID = 0;
        int matchID = 0;
        int matchLevelID = 0;
        float best = 0;
        float progressRunSnapshot = 0;
        float progressRunElapsed = 0;
        std::array<size_t, 100> pendingDeathCount = {};
        std::vector<float> pendingDeathProgresses;
        std::vector<ScoreSubmission> pendingScoreSubmissions;
        std::atomic<bool> deathSubmitInFlight{false};
        std::atomic<bool> scoreSubmitInFlight{false};
        bool platformer = false;
        bool completionPending = false;
        bool progressRunActive = false;
        bool progressRunAnnounced = false;
        bool scoreMode = false;
        bool matchLookupCompleted = false;
        std::string playMode = "normal";
        std::string submittedPlayMode;
        std::atomic<bool> inPvp{false};

        explicit State(int levelID = 0) : levelID(levelID) {
        }
    };

    std::shared_ptr<State> m_state;

    void submit(bool completed = false);
    static void submitProgress(std::shared_ptr<State> state, bool completed = false);
    static void submitDeathCount(std::shared_ptr<State> state);
    static void submitDeathProgress(std::shared_ptr<State> state, float progress);
    static void announceProgressRun(std::shared_ptr<State> state, float progress, float progressSpeed);
    static void submitScoreSubmission(std::shared_ptr<State> state);
    static void submitPlayMode(std::shared_ptr<State> state, std::string const& playMode);
    static bool isLevelValid(std::shared_ptr<State> state);
    static std::string serializeDeathCount(std::array<size_t, 100> const& count);
    static size_t sumDeathCount(std::array<size_t, 100> const& count);

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
    void resetProgressState();
    void recordCheckpoint(int count);
    void completePlatformer(int count);
};
