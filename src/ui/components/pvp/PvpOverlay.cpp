#include "PvpOverlay.hpp"

PvpOverlay::PvpOverlay(PlayLayer* layer) : m_layer(layer) {
    if (!m_layer) {
        return;
    }

    m_label = CCLabelBMFont::create("Versus\nYou: 0.00%\nOpponent: 0.00%", "bigFont.fnt");
    m_label->setScale(0.32f);
    m_label->setOpacity(180);
    m_label->setVisible(false);
    m_label->setID("gdvn-pvp-overlay"_spr);

    auto parent = m_layer->m_uiLayer ? static_cast<CCNode*>(m_layer->m_uiLayer) : static_cast<CCNode*>(m_layer);
    parent->addChild(m_label, 1000);
    this->updatePosition();
}

PvpOverlay::~PvpOverlay() {
    this->cleanup();
}

void PvpOverlay::setText(std::string const& text) {
    if (m_label) {
        m_label->setString(text.c_str());
    }
}

void PvpOverlay::setVisible(bool visible) {
    if (m_label) {
        m_label->setVisible(visible && Mod::get()->getSettingValue<bool>("show-pvp-overlay"));
    }
}

void PvpOverlay::updatePosition() {
    if (!m_label) {
        return;
    }

    auto size = CCDirector::sharedDirector()->getWinSize();
    m_label->setAnchorPoint({0.0f, 1.0f});
    m_label->setPosition({LABEL_MARGIN, size.height - LABEL_MARGIN});
}

void PvpOverlay::cleanup() {
    if (m_label) {
        m_label->removeFromParentAndCleanup(true);
        m_label = nullptr;
    }
}
