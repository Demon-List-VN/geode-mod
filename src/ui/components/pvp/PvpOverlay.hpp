#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>

#include <string>

using namespace geode::prelude;

class PvpOverlay {
  public:
    explicit PvpOverlay(PlayLayer* layer);
    ~PvpOverlay();

    void setText(std::string const& text);
    void setVisible(bool visible);
    void updatePosition();
    void cleanup();

  private:
    static constexpr float LABEL_MARGIN = 8.0f;

    PlayLayer* m_layer = nullptr;
    CCLabelBMFont* m_label = nullptr;
};
