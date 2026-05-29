#pragma once

#include "../dtos/level/LevelInfoResponseDto.hpp"
#include "LevelListAdapter.hpp"
#include <Geode/Geode.hpp>

class LevelInfoResponseAdapter {
  public:
    static LevelInfoResponseDto fromJson(matjson::Value const& json) {
        LevelInfoResponseDto dto;

        if (!json.isArray()) {
            return dto;
        }

        dto.valid = true;

        for (auto const& item : json.asArray().unwrap()) {
            if (item.isObject()) {
                dto.lists.push_back(LevelListAdapter::fromJson(item));
            }
        }

        return dto;
    }
};

