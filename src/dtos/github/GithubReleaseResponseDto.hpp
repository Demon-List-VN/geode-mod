#pragma once

#include <string>

struct GithubReleaseResponseDto {
	bool valid = false;
	std::string tagName;
};

