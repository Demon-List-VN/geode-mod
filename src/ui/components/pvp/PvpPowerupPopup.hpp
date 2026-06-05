#pragma once

#include "../../../dtos/pvp/PvpPowerupStateDto.hpp"
#include "../../../models/pvp/overlay/PvpOverlayPlayerProgressModel.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/ui/Popup.hpp>

#include <string>
#include <vector>

using namespace geode::prelude;

class PvpOverlayService;

class PvpPowerupPopup final : public Popup {
  public:
    static PvpPowerupPopup* create(PvpOverlayService* overlay);

    void refresh();
    void applyState(PvpPowerupStateDto const& state);
    void closeFromOverlay();

  protected:
    bool init(PvpOverlayService* overlay);
    void onClose(CCObject* sender) override;

  private:
    PvpOverlayService* m_overlay = nullptr;
    PvpPowerupStateDto m_state;
    std::vector<PvpOverlayPlayerProgressModel> m_targets;
    std::vector<CCMenuItemSpriteExtra*> m_targetButtons;
    CCLabelBMFont* m_manaLabel = nullptr;
    CCLabelBMFont* m_statusLabel = nullptr;
    CCMenuItemSpriteExtra* m_flashButton = nullptr;
    CCMenuItemSpriteExtra* m_invisibleButton = nullptr;
    CCMenuItemSpriteExtra* m_shieldButton = nullptr;
    std::string m_selectedTargetUid;
    bool m_randomTarget = true;
    bool m_loading = false;

    void rebuildTargets();
    void updateControls();
    void setStatus(std::string const& text);
    void setLoading(bool loading);
    void selectRandomTarget();
    void selectTarget(std::string uid);
    void cast(std::string const& skill);
    int skillCost(std::string const& skill) const;
    bool harmfulTargetAvailable() const;
};
