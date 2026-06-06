#include "PvpPowerupPopup.hpp"

#include "../../../services/pvp/PvpOverlayService.hpp"
#include "../../../utils/StringUtils.hpp"
#include <Geode/binding/ButtonSprite.hpp>

#include <algorithm>
#include <utility>

namespace {
std::string targetLabel(PvpOverlayPlayerProgressModel const& target, int index) {
    if (!target.name.empty()) {
        return gdvn::utils::string::truncate(target.name, 12);
    }

    return "Player " + std::to_string(index + 1);
}
} // namespace

PvpPowerupPopup* PvpPowerupPopup::create(PvpOverlayService* overlay) {
    auto ret = new PvpPowerupPopup();
    if (ret->init(overlay)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool PvpPowerupPopup::init(PvpOverlayService* overlay) {
    if (!Popup::init(410.0f, 270.0f)) {
        return false;
    }

    m_overlay = overlay;
    this->setTitle("Versus Skills");

    m_manaLabel = CCLabelBMFont::create("Mana: --/100", "bigFont.fnt");
    m_manaLabel->setScale(0.42f);
    m_mainLayer->addChildAtPosition(m_manaLabel, Anchor::Top, {0.0f, -36.0f});

    auto targetTitle = CCLabelBMFont::create("Target", "bigFont.fnt");
    targetTitle->setScale(0.32f);
    targetTitle->setOpacity(180);
    m_mainLayer->addChildAtPosition(targetTitle, Anchor::TopLeft, {24.0f, -62.0f});

    auto skillTitle = CCLabelBMFont::create("Skill", "bigFont.fnt");
    skillTitle->setScale(0.32f);
    skillTitle->setOpacity(180);
    m_mainLayer->addChildAtPosition(skillTitle, Anchor::TopLeft, {24.0f, -128.0f});

    auto flashSprite = ButtonSprite::create("Flash 55", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    flashSprite->setScale(0.48f);
    m_flashButton = CCMenuItemExt::createSpriteExtra(flashSprite, [this](auto*) { this->cast("flashbang"); });
    m_buttonMenu->addChildAtPosition(m_flashButton, Anchor::BottomLeft, {62.0f, 72.0f});

    auto invisibleSprite = ButtonSprite::create("Invis 40", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    invisibleSprite->setScale(0.48f);
    m_invisibleButton = CCMenuItemExt::createSpriteExtra(invisibleSprite, [this](auto*) { this->cast("invisible"); });
    m_buttonMenu->addChildAtPosition(m_invisibleButton, Anchor::Bottom, {0.0f, 72.0f});

    auto shieldSprite = ButtonSprite::create("Shield 45", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    shieldSprite->setScale(0.48f);
    m_shieldButton = CCMenuItemExt::createSpriteExtra(shieldSprite, [this](auto*) { this->cast("shield"); });
    m_buttonMenu->addChildAtPosition(m_shieldButton, Anchor::BottomRight, {-62.0f, 72.0f});

    auto pauseSprite = ButtonSprite::create("Pause 35", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    pauseSprite->setScale(0.48f);
    m_pauseButton = CCMenuItemExt::createSpriteExtra(pauseSprite, [this](auto*) { this->cast("pause"); });
    m_buttonMenu->addChildAtPosition(m_pauseButton, Anchor::BottomLeft, {62.0f, 38.0f});

    auto doubleClickSprite = ButtonSprite::create("2Click 65", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    doubleClickSprite->setScale(0.48f);
    m_doubleClickButton = CCMenuItemExt::createSpriteExtra(doubleClickSprite, [this](auto*) {
        this->cast("double_click");
    });
    m_buttonMenu->addChildAtPosition(m_doubleClickButton, Anchor::Bottom, {0.0f, 38.0f});

    auto resetSprite = ButtonSprite::create("Reset 100", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    resetSprite->setScale(0.48f);
    m_forceResetButton = CCMenuItemExt::createSpriteExtra(resetSprite, [this](auto*) { this->cast("force_reset"); });
    m_buttonMenu->addChildAtPosition(m_forceResetButton, Anchor::BottomRight, {-62.0f, 38.0f});

    m_statusLabel = CCLabelBMFont::create("Loading...", "bigFont.fnt");
    m_statusLabel->setScale(0.3f);
    m_statusLabel->setOpacity(190);
    m_mainLayer->addChildAtPosition(m_statusLabel, Anchor::Bottom, {0.0f, 14.0f});

    this->rebuildTargets();
    this->updateControls();
    return true;
}

void PvpPowerupPopup::refresh() {
    if (!m_overlay || m_loading) {
        return;
    }

    m_targets = m_overlay->powerupTargets();
    this->rebuildTargets();
    this->setLoading(true);
    this->setStatus("Loading...");
    this->retain();
    m_overlay->requestPowerupState([this](PvpPowerupStateDto const& state, bool ok) {
        if (m_overlay && ok) {
            m_state = state;
            this->setStatus("");
        } else if (m_overlay) {
            this->setStatus("Failed to load skills");
        }
        this->setLoading(false);
        this->updateControls();
        this->release();
    });
}

void PvpPowerupPopup::applyState(PvpPowerupStateDto const& state) {
    if (!state.valid) {
        return;
    }

    m_state = state;
    this->updateControls();
}

void PvpPowerupPopup::closeFromOverlay() {
    m_overlay = nullptr;
    this->onClose(nullptr);
}

void PvpPowerupPopup::onClose(CCObject* sender) {
    if (m_overlay) {
        auto overlay = m_overlay;
        m_overlay = nullptr;
        overlay->notifyPowerupPopupClosed(this);
    }

    Popup::onClose(sender);
}

void PvpPowerupPopup::rebuildTargets() {
    for (auto* button : m_targetButtons) {
        if (button) {
            button->removeFromParentAndCleanup(true);
        }
    }
    m_targetButtons.clear();

    auto randomSprite = ButtonSprite::create(m_randomTarget ? "Random *" : "Random", "goldFont.fnt",
                                             "GJ_button_01.png", 0.8f);
    randomSprite->setScale(0.42f);
    auto randomButton = CCMenuItemExt::createSpriteExtra(randomSprite, [this](auto*) { this->selectRandomTarget(); });
    m_buttonMenu->addChildAtPosition(randomButton, Anchor::TopLeft, {65.0f, -86.0f});
    m_targetButtons.push_back(randomButton);

    for (size_t i = 0; i < m_targets.size(); ++i) {
        auto const& target = m_targets[i];
        auto selected = !m_randomTarget && target.uid == m_selectedTargetUid;
        auto label = targetLabel(target, static_cast<int>(i)) + (selected ? " *" : "");
        auto sprite = ButtonSprite::create(label.c_str(), "goldFont.fnt", "GJ_button_01.png", 0.8f);
        sprite->setScale(0.38f);
        auto uid = target.uid;
        auto button = CCMenuItemExt::createSpriteExtra(sprite, [this, uid](auto*) { this->selectTarget(uid); });
        auto col = static_cast<float>(i % 3);
        auto row = static_cast<float>(i / 3);
        m_buttonMenu->addChildAtPosition(button, Anchor::TopLeft, {150.0f + col * 70.0f, -86.0f - row * 30.0f});
        m_targetButtons.push_back(button);
    }

    this->updateControls();
}

void PvpPowerupPopup::updateControls() {
    if (m_manaLabel) {
        m_manaLabel->setString(fmt::format("Mana: {}/{}", m_state.mana, m_state.maxMana).c_str());
    }

    auto hasTarget = this->harmfulTargetAvailable();
    if (m_flashButton) {
        m_flashButton->setEnabled(!m_loading && hasTarget && m_state.mana >= this->skillCost("flashbang"));
    }
    if (m_invisibleButton) {
        m_invisibleButton->setEnabled(!m_loading && hasTarget && m_state.mana >= this->skillCost("invisible"));
    }
    if (m_shieldButton) {
        m_shieldButton->setEnabled(!m_loading && !m_state.shieldActive && m_state.mana >= this->skillCost("shield"));
    }
    if (m_pauseButton) {
        m_pauseButton->setEnabled(!m_loading && hasTarget && m_state.mana >= this->skillCost("pause"));
    }
    if (m_doubleClickButton) {
        m_doubleClickButton->setEnabled(!m_loading && hasTarget && m_state.mana >= this->skillCost("double_click"));
    }
    if (m_forceResetButton) {
        m_forceResetButton->setEnabled(!m_loading && hasTarget && m_state.mana >= this->skillCost("force_reset"));
    }
}

void PvpPowerupPopup::setStatus(std::string const& text) {
    if (m_statusLabel) {
        m_statusLabel->setString(text.c_str());
    }
}

void PvpPowerupPopup::setLoading(bool loading) {
    m_loading = loading;
    this->updateControls();
}

void PvpPowerupPopup::selectRandomTarget() {
    m_randomTarget = true;
    m_selectedTargetUid.clear();
    this->rebuildTargets();
}

void PvpPowerupPopup::selectTarget(std::string uid) {
    m_randomTarget = false;
    m_selectedTargetUid = std::move(uid);
    this->rebuildTargets();
}

void PvpPowerupPopup::cast(std::string const& skill) {
    if (!m_overlay || m_loading) {
        return;
    }

    if (skill != "shield" && !this->harmfulTargetAvailable()) {
        this->setStatus("Select a target");
        return;
    }

    auto targetUid = skill == "shield"
        ? m_overlay->currentUid()
        : (m_randomTarget ? std::string() : m_selectedTargetUid);
    auto randomTarget = skill != "shield" && m_randomTarget;

    this->setLoading(true);
    this->setStatus("Casting...");
    this->retain();
    m_overlay->castPowerupSkill(skill, targetUid, randomTarget,
                                [this, skill](PvpPowerupCastResponseDto const& response, bool ok) {
                                    if (m_overlay && ok) {
                                        m_state = response.state;
                                        this->setStatus(skill == "shield" ? "Shield ready" : "Cast sent");
                                    } else if (m_overlay) {
                                        this->setStatus("Cast failed");
                                    }
                                    this->setLoading(false);
                                    this->updateControls();
                                    this->release();
                                });
}

int PvpPowerupPopup::skillCost(std::string const& skill) const {
    auto found = std::find_if(m_state.skills.begin(), m_state.skills.end(),
                              [&skill](PvpPowerupSkillDto const& item) { return item.skill == skill; });

    if (found != m_state.skills.end() && found->cost > 0) {
        return found->cost;
    }

    if (skill == "flashbang") {
        return 55;
    }
    if (skill == "invisible") {
        return 40;
    }
    if (skill == "pause") {
        return 35;
    }
    if (skill == "double_click") {
        return 65;
    }
    if (skill == "force_reset") {
        return 100;
    }
    return 45;
}

bool PvpPowerupPopup::harmfulTargetAvailable() const {
    return m_randomTarget ? !m_targets.empty() : !m_selectedTargetUid.empty();
}
