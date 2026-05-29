#pragma once

#include "../dtos/pvp/PvpMessagesResponseDto.hpp"
#include "PvpMessageAdapter.hpp"
#include <Geode/Geode.hpp>

class PvpMessagesResponseAdapter {
  public:
    static PvpMessagesResponseDto fromJson(matjson::Value const& json) {
        PvpMessagesResponseDto dto;

        if (json.isArray()) {
            addMessages(dto, json);
        } else if (json["messages"].isArray()) {
            addMessages(dto, json["messages"]);
        } else if (json["data"].isArray()) {
            addMessages(dto, json["data"]);
        }

        dto.valid = true;
        return dto;
    }

  private:
    static void addMessages(PvpMessagesResponseDto& dto, matjson::Value const& json) {
        for (auto const& message : json.asArray().unwrap()) {
            auto mapped = PvpMessageAdapter::fromJson(message);
            if (mapped.valid) {
                dto.messages.push_back(mapped);
            }
        }
    }
};

