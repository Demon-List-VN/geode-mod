#pragma once

#include "../../../models/pvp/overlay/PvpOverlayRecentChatMessageModel.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>

#include <cstdint>
#include <string>
#include <vector>

using namespace geode::prelude;

class PvpRecentChatStack {
  public:
    explicit PvpRecentChatStack(PlayLayer* layer);
    ~PvpRecentChatStack();

    void pushMessage(std::int64_t id, std::string const& text, std::string const& type, bool sentByCurrentUser);
    void update(float dt);
    void setVisible(bool visible);
    bool hasMessages() const;
    void updatePosition();
    void clear();
    void cleanup();

  private:
    static constexpr float CHAT_MARGIN = 8.0f;
    static constexpr float CHAT_MESSAGE_LIFETIME = 8.0f;
    static constexpr float CHAT_FADE_TIME = 1.5f;
    static constexpr int MAX_RECENT_MESSAGES = 4;

    CCNode* m_stack = nullptr;
    std::vector<PvpOverlayRecentChatMessageModel> m_messages;
    std::vector<CCLabelBMFont*> m_labels;

    void layoutMessages();
};
