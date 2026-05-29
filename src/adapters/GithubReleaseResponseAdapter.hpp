#pragma once

#include "../dtos/github/GithubReleaseResponseDto.hpp"
#include <Geode/Geode.hpp>

namespace gdvn::adapters {

class GithubReleaseResponseAdapter {
  public:
    static GithubReleaseResponseDto fromJson(matjson::Value const& json) {
        GithubReleaseResponseDto dto;

        if (json["tag_name"].isString()) {
            dto.valid = true;
            dto.tagName = json["tag_name"].asString().unwrapOrDefault();
        }

        return dto;
    }
};

} // namespace gdvn::adapters
