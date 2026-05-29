#pragma once

#include <Geode/Geode.hpp>
#include <string>

namespace gdvn::auth_config {

inline std::string getToken() {
    return Mod::get()->getSavedValue<std::string>("api-key");
}

} // namespace gdvn::auth_config
