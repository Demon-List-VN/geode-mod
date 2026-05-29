#pragma once

#include <Geode/Geode.hpp>
#include "../dtos/level/LevelListDto.hpp"

namespace gdvn::level_info {

geode::prelude::CCLabelBMFont* createLabel(GJGameLevel* level, std::string str, int order);
std::vector<std::string> getListInfoLabels(
	std::vector<LevelListDto> const& lists,
	bool isLoggedIn
);

class ButtonCreator {
public:
	void onButton(geode::prelude::CCObject* sender);
	geode::prelude::CCMenu* create(
		std::vector<std::string> labels,
		GJGameLevel* level,
		geode::prelude::CCLayer* layer
	);
};

}
