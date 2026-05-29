#include "LevelClient.hpp"

#include "../../adapters/ActivePvpMatchResponseAdapter.hpp"
#include "../../adapters/LevelInfoResponseAdapter.hpp"
#include "../../common.hpp"
#include "../../utils/AuthConfig.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
}

void LevelClient::getLevel(int id, GetLevelCallback callback) {
    web::WebRequest req;
    auto token = gdvn::auth_config::getToken();

    if (!token.empty()) {
        req.header("Authorization", "Bearer " + token);
    }

    auto url = API_URL + "/lists/levels/" + std::to_string(id) + "/starred";

    s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        LevelInfoResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = LevelInfoResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}

void LevelClient::getActivePvpMatch(int levelID, GetActivePvpMatchCallback callback) {
    web::WebRequest req;
    std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inPvp";

    req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

    s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        ActivePvpMatchResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = ActivePvpMatchResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}
