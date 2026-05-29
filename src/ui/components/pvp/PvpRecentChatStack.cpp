#include "PvpRecentChatStack.hpp"

#include <algorithm>
#include <cstddef>

PvpRecentChatStack::PvpRecentChatStack(PlayLayer* layer) {
    if (!layer) {
        return;
    }

    auto parent = layer->m_uiLayer ? static_cast<CCNode*>(layer->m_uiLayer) : static_cast<CCNode*>(layer);

    m_stack = CCNode::create();
    m_stack->setID("gdvn-pvp-chat-stack"_spr);
    m_stack->setVisible(false);
    parent->addChild(m_stack, 1001);

    this->updatePosition();
}

PvpRecentChatStack::~PvpRecentChatStack() {
    this->cleanup();
}

void PvpRecentChatStack::pushMessage(std::int64_t id, std::string const& text, std::string const& type,
                                     bool sentByCurrentUser) {
    if (!m_stack || text.empty()) {
        return;
    }

    auto label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
    label->setAnchorPoint({0.0f, 0.0f});
    label->setScale(0.55f);
    label->setOpacity(220);

    if (type == "system") {
        label->setColor(ccc3(255, 225, 120));
    } else if (sentByCurrentUser) {
        label->setColor(ccc3(150, 255, 150));
    }

    label->limitLabelWidth(280.0f, 0.55f, 0.3f);
    m_stack->addChild(label);

    m_messages.push_back({id, CHAT_MESSAGE_LIFETIME});
    m_labels.push_back(label);

    while (m_messages.size() > MAX_RECENT_MESSAGES) {
        if (!m_labels.empty()) {
            auto* oldLabel = m_labels.front();
            m_labels.erase(m_labels.begin());
            if (oldLabel) {
                oldLabel->removeFromParentAndCleanup(true);
            }
        }
        m_messages.erase(m_messages.begin());
    }

    this->layoutMessages();
}

void PvpRecentChatStack::update(float dt) {
    if (m_messages.empty()) {
        return;
    }

    for (size_t i = 0; i < m_messages.size(); ++i) {
        auto& message = m_messages[i];
        message.timeLeft -= dt;
        if (i < m_labels.size() && m_labels[i]) {
            auto opacity = message.timeLeft < CHAT_FADE_TIME
                               ? static_cast<unsigned char>(std::max(0.0f, message.timeLeft / CHAT_FADE_TIME) * 220.0f)
                               : static_cast<unsigned char>(220);
            m_labels[i]->setOpacity(opacity);
        }
    }

    for (size_t i = 0; i < m_messages.size();) {
        if (m_messages[i].timeLeft <= 0.0f) {
            if (i < m_labels.size()) {
                if (auto* label = m_labels[i]) {
                    label->removeFromParentAndCleanup(true);
                }
                m_labels.erase(m_labels.begin() + static_cast<std::ptrdiff_t>(i));
            }
            m_messages.erase(m_messages.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    this->layoutMessages();
}

void PvpRecentChatStack::setVisible(bool visible) {
    if (m_stack) {
        m_stack->setVisible(visible);
    }
}

bool PvpRecentChatStack::hasMessages() const {
    return !m_messages.empty();
}

void PvpRecentChatStack::updatePosition() {
    if (m_stack) {
        m_stack->setPosition({CHAT_MARGIN, CHAT_MARGIN});
    }
}

void PvpRecentChatStack::clear() {
    for (auto*& label : m_labels) {
        if (label) {
            label->removeFromParentAndCleanup(true);
            label = nullptr;
        }
    }
    m_labels.clear();
    m_messages.clear();
}

void PvpRecentChatStack::cleanup() {
    if (m_stack) {
        m_stack->removeFromParentAndCleanup(true);
        m_stack = nullptr;
    }
    m_labels.clear();
    m_messages.clear();
}

void PvpRecentChatStack::layoutMessages() {
    for (size_t i = 0; i < m_messages.size(); ++i) {
        if (i < m_labels.size()) {
            auto* label = m_labels[i];
            auto reverseIndex = m_messages.size() - i - 1;
            if (label) {
                label->setPosition({0.0f, static_cast<float>(reverseIndex) * 12.0f});
            }
        }
    }
}
