#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <cstddef>
#include <string>
#include <vector>

using namespace geode::prelude;

class PvpOverlayService;

class PvpChatPopup final : public Popup, public TextInputDelegate {
  public:
    static PvpChatPopup* create(PvpOverlayService* overlay);

    void focusInput();
    void didSend();
    void closeFromOverlay();
    void updateHistory();
    void setSending(bool sending);
    void setStatus(std::string const& status);

  protected:
    bool init(PvpOverlayService* overlay);
    void onClose(CCObject* sender) override;
    void textChanged(CCTextInputNode*) override;
    void textInputReturn(CCTextInputNode*) override;
    void enterPressed(CCTextInputNode*) override;

  private:
    static constexpr float CHAT_HISTORY_WIDTH = 320.0f;
    static constexpr float CHAT_HISTORY_LINE_HEIGHT = 15.0f;
    static constexpr int CHAT_HISTORY_VISIBLE_LINES = 8;
    static constexpr int MAX_CHAT_MESSAGE_LENGTH = 500;

    PvpOverlayService* m_overlay = nullptr;
    TextInput* m_input = nullptr;
    std::vector<CCLabelBMFont*> m_historyLabels;
    CCLabelBMFont* m_statusLabel = nullptr;
    CCMenuItemSpriteExtra* m_sendButton = nullptr;
    CCMenuItemSpriteExtra* m_upButton = nullptr;
    CCMenuItemSpriteExtra* m_downButton = nullptr;
    size_t m_historyOffset = 0;
    bool m_sending = false;

    void scrollHistory(int delta);
    void submit();
};
