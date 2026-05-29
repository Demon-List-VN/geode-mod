#pragma once

#include <Geode/Geode.hpp>
#include <atomic>
#include <memory>

using namespace geode::prelude;

class EventSubmitterService {
    struct State {
        int levelID = 0;
        float best = 0;
        std::atomic<bool> inEvent{false};

        explicit State(int levelID = 0) : levelID(levelID) {
        }
    };

    std::shared_ptr<State> m_state;

    void submit();

  public:
    EventSubmitterService();
    ~EventSubmitterService();
    EventSubmitterService(int levelID);
    void record(float progress);
};
