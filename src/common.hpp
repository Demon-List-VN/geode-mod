#pragma once

#include <string>

#if BUILD_RELWITHDEBINFO
    const std::string API_URL = "http://localhost:8787";
#else
    const std::string API_URL = "https://api.gdvn.net";
#endif

const std::string EVENT_PASSWORD = "69229623652108781802661011115864";