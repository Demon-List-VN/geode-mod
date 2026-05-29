#pragma once

#include "LevelListItemDto.hpp"
#include <string>

struct LevelListDto {
    std::string slug;
    std::string title;
    std::string mode;
    bool isOfficial = false;
    bool starred = false;
    bool hasStarred = false;
    bool topEnabled = false;
    bool hasTopEnabled = false;
    LevelListItemDto item;

    std::string label() const {
        if (!slug.empty()) {
            return slug;
        }

        if (!title.empty()) {
            return title;
        }

        return "List";
    }

    bool isStarredList() const {
        if (hasStarred) {
            return starred;
        }

        return !isOfficial;
    }

    bool isTopMode() const {
        if (hasTopEnabled) {
            return topEnabled;
        }

        return mode == "top";
    }
};
