#include "../services/pvp/PvpOverlayService.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp> // DO NOT REMOVE

using namespace geode::prelude;

class $modify(GDVNPlayerObject, PlayerObject) {
    bool pushButton(PlayerButton button) {
        auto overlay = PvpOverlayService::getActive();
        if (overlay && overlay->shouldBlockButtonDown(static_cast<int>(button), !m_isSecondPlayer)) {
            return false;
        }

        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        auto overlay = PvpOverlayService::getActive();
        if (overlay && overlay->shouldBlockButtonRelease(static_cast<int>(button), !m_isSecondPlayer)) {
            return false;
        }

        return PlayerObject::releaseButton(button);
    }
};
