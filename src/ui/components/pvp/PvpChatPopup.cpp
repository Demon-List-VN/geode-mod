#include "PvpChatPopup.hpp"

#include "../../../services/pvp/PvpOverlayService.hpp"
#include "../../../utils/StringUtils.hpp"
#include <Geode/binding/ButtonSprite.hpp>

#include <algorithm>
#include <utility>

PvpChatPopup* PvpChatPopup::create(PvpOverlayService* overlay) {
    auto ret = new PvpChatPopup();
    if (ret->init(overlay)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void PvpChatPopup::focusInput() {
    if (m_input) {
        m_input->focus();
    }
}

void PvpChatPopup::didSend() {
    m_sending = false;
    this->setSending(false);
    this->setStatus("");

    if (m_input) {
        m_input->setString("", false);
        m_input->focus();
    }

    this->updateHistory();
}

void PvpChatPopup::closeFromOverlay() {
    m_overlay = nullptr;
    this->onClose(nullptr);
}

void PvpChatPopup::updateHistory() {
    if (!m_overlay) {
        return;
    }

    auto lines = m_overlay->getChatHistoryLines();
    if (lines.empty()) {
        lines.push_back("No messages yet.");
    }

    auto maxOffset = lines.size() > CHAT_HISTORY_VISIBLE_LINES ? lines.size() - CHAT_HISTORY_VISIBLE_LINES : 0;
    m_historyOffset = std::min(m_historyOffset, maxOffset);
    auto start = maxOffset - m_historyOffset;
    auto end = std::min(lines.size(), start + CHAT_HISTORY_VISIBLE_LINES);

    for (auto* label : m_historyLabels) {
        if (label) {
            label->removeFromParentAndCleanup(true);
        }
    }
    m_historyLabels.clear();

    for (size_t i = start; i < end; ++i) {
        auto label = CCLabelBMFont::create(lines[i].c_str(), "chatFont.fnt");
        label->setAnchorPoint({0.0f, 1.0f});
        label->setScale(0.52f);
        label->setOpacity(220);
        label->setColor(ccc3(235, 235, 235));
        label->limitLabelWidth(CHAT_HISTORY_WIDTH, 0.52f, 0.28f);
        m_mainLayer->addChildAtPosition(label, Anchor::TopLeft,
                                        {24.0f, -38.0f - CHAT_HISTORY_LINE_HEIGHT * static_cast<float>(i - start)});
        m_historyLabels.push_back(label);
    }

    if (m_upButton) {
        m_upButton->setEnabled(m_historyOffset < maxOffset);
    }
    if (m_downButton) {
        m_downButton->setEnabled(m_historyOffset > 0);
    }
}

bool PvpChatPopup::init(PvpOverlayService* overlay) {
    if (!Popup::init(380.0f, 220.0f)) {
        return false;
    }

    m_overlay = overlay;
    this->setTitle("Versus Chat");

    auto upSprite = ButtonSprite::create("Up", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    upSprite->setScale(0.4f);
    m_upButton = CCMenuItemExt::createSpriteExtra(upSprite, [this](auto*) { this->scrollHistory(1); });
    m_buttonMenu->addChildAtPosition(m_upButton, Anchor::TopRight, {-28.0f, -48.0f});

    auto downSprite = ButtonSprite::create("Down", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    downSprite->setScale(0.4f);
    m_downButton = CCMenuItemExt::createSpriteExtra(downSprite, [this](auto*) { this->scrollHistory(-1); });
    m_buttonMenu->addChildAtPosition(m_downButton, Anchor::TopRight, {-28.0f, -86.0f});

    m_input = TextInput::create(250.0f, "Type a message...", "chatFont.fnt");
    m_input->setCommonFilter(CommonFilter::Any);
    m_input->setMaxCharCount(MAX_CHAT_MESSAGE_LENGTH);
    m_input->setTextAlign(TextInputAlign::Left);
    m_input->setDelegate(this);
    m_mainLayer->addChildAtPosition(m_input, Anchor::Bottom, {-38.0f, 38.0f});

    auto sendSprite = ButtonSprite::create("Send", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    sendSprite->setScale(0.55f);
    m_sendButton = CCMenuItemExt::createSpriteExtra(sendSprite, [this](auto*) { this->submit(); });
    m_buttonMenu->addChildAtPosition(m_sendButton, Anchor::Bottom, {128.0f, 38.0f});

    m_statusLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_statusLabel->setScale(0.32f);
    m_statusLabel->setOpacity(180);
    m_mainLayer->addChildAtPosition(m_statusLabel, Anchor::Bottom, {0.0f, 16.0f});

    this->updateHistory();

    return true;
}

void PvpChatPopup::onClose(CCObject* sender) {
    for (auto*& label : m_historyLabels) {
        if (label) {
            label->removeFromParentAndCleanup(true);
            label = nullptr;
        }
    }
    m_historyLabels.clear();

    if (m_overlay) {
        auto overlay = m_overlay;
        m_overlay = nullptr;
        overlay->notifyChatPopupClosed(this);
    }

    Popup::onClose(sender);
}

void PvpChatPopup::textChanged(CCTextInputNode*) {
}

void PvpChatPopup::textInputReturn(CCTextInputNode*) {
    this->submit();
}

void PvpChatPopup::enterPressed(CCTextInputNode*) {
    this->submit();
}

void PvpChatPopup::setSending(bool sending) {
    m_sending = sending;
    if (m_sendButton) {
        m_sendButton->setEnabled(!sending);
    }
}

void PvpChatPopup::setStatus(std::string const& status) {
    if (m_statusLabel) {
        m_statusLabel->setString(status.c_str());
    }
}

void PvpChatPopup::scrollHistory(int delta) {
    auto lineCount = m_overlay ? m_overlay->getChatHistoryLines().size() : 0;
    auto maxOffset = lineCount > CHAT_HISTORY_VISIBLE_LINES ? lineCount - CHAT_HISTORY_VISIBLE_LINES : 0;
    auto next = static_cast<int>(m_historyOffset) + delta;
    m_historyOffset = static_cast<size_t>(std::clamp(next, 0, static_cast<int>(maxOffset)));
    this->updateHistory();
}

void PvpChatPopup::submit() {
    if (!m_overlay || !m_input || m_sending) {
        return;
    }

    auto content = gdvn::utils::string::trimCopy(static_cast<std::string>(m_input->getString()));
    if (content.empty()) {
        this->setStatus("Message cannot be empty");
        return;
    }

    this->setSending(true);
    this->setStatus("Sending...");
    m_overlay->submitChatMessage(std::move(content));
}
