#include "DeathData.hpp"

DeathData::DeathData() {
}

DeathData::DeathData(int id, bool a, std::array<size_t, 100> b) {
    levelID = id;
    completed = a;
    cnt = b;
}

std::string DeathData::serialize() {
    std::string res;

    for (size_t i : cnt) {
        res += std::to_string(i) + '|';
    }

    res.pop_back();

    return res;
}

void DeathData::addDeathCount(int percent) {
    if (completed) {
        return;
    }

    if (!(0 <= percent && percent < 100)) {
        return;
    }

    cnt[percent]++;
}

bool DeathData::isCompleted() {
    return completed;
}

void DeathData::setCompleted() {
    completed = true;
}