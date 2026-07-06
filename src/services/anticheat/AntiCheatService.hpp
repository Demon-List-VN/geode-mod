#pragma once

#include "../../interfaces/AntiCheatInterface.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>

#include "DamageBypassAntiCheatService.hpp"
#include "EclipseAntiCheatService.hpp"
#include "NoclipAntiCheatService.hpp"
#include "OpenHackAntiCheatService.hpp"
#include "PrismAntiCheatService.hpp"
#include "QolModAntiCheatService.hpp"
#include "StartPositionAntiCheatService.hpp"

#include <optional>
#include <string>
#include <string_view>

class AntiCheatService : public AntiCheatInterface {
  private:
    PlayLayer* playLayer = nullptr;
    mutable bool reportedCheat = false;
    bool checksDisabled = false;
    mutable std::string reportedCheatReason;
    StartPositionAntiCheatService startPositionAntiCheat;
    EclipseAntiCheatService eclipseAntiCheat;
    QolModAntiCheatService qolModAntiCheat;
    PrismAntiCheatService prismAntiCheat;
    OpenHackAntiCheatService openHackAntiCheat;
    DamageBypassAntiCheatService damageBypassAntiCheat;
    NoclipAntiCheatService noclipAntiCheat;

    void markRunCheated(std::string const& reason) const;
    void markRunCheatedIfNeeded() const;

  public:
    AntiCheatService();
    explicit AntiCheatService(PlayLayer* playLayer);

    void reset(PlayLayer* playLayer) override;
    void disableChecks();
    void onUpdate(float dt) override;
    void onPlayerDestroyed(PlayerObject* player) override;
    bool isCheated() const override;
    std::optional<std::string_view> getCheatedReason() const;
    std::string_view getCheatReason() const override;
};
