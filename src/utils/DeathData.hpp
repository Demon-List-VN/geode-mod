#pragma once

class DeathData {
  public:
    int levelID = 0;
    bool completed = false;
    std::array<size_t, 100> cnt = {};

    DeathData();
    DeathData(int id, bool a, std::array<size_t, 100> b);
    std::string serialize();
    void addDeathCount(int percent);
    bool isCompleted();
    void setCompleted();
};