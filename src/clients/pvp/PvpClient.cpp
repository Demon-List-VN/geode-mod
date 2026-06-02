#include "PvpClient.hpp"

#include "../../adapters/PvpMatchAdapter.hpp"
#include "../../adapters/PvpMessageAdapter.hpp"
#include "../../adapters/PvpMessagesResponseAdapter.hpp"
#include "../../consts/ConfigConst.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace {
std::string singleDeathCount(float progress) {
    std::string count;
    const int percent = std::clamp(static_cast<int>(std::floor(progress)), 0, 99);

    for (int i = 0; i < 100; i++) {
        count += i == percent ? "1" : "0";
        if (i < 99) {
            count += "|";
        }
    }

    return count;
}
} // namespace

async::TaskHolder<web::WebResponse> PvpClient::s_putPlayModeHolder;
async::TaskHolder<web::WebResponse> PvpClient::s_putProgressHolder;
std::vector<std::shared_ptr<async::TaskHolder<web::WebResponse>>> PvpClient::s_postHolders;
std::vector<std::shared_ptr<async::TaskHolder<web::WebResponse>>> PvpClient::s_getMatchHolders;
async::TaskHolder<web::WebResponse> PvpClient::s_getHolder;

void PvpClient::getMatch(int matchID, GetMatchCallback callback) {
    web::WebRequest req;
    auto url = gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID);

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    auto holder = std::make_shared<async::TaskHolder<web::WebResponse>>();
    PvpClient::s_getMatchHolders.push_back(holder);
    holder->spawn(req.get(url), [callback, holder](web::WebResponse res) {
        auto& holders = PvpClient::s_getMatchHolders;
        holders.erase(std::remove(holders.begin(), holders.end(), holder), holders.end());

        PvpMatchSnapshotDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = PvpMatchAdapter::matchSnapshotFromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}

void PvpClient::putPlayMode(int matchID, std::string const& playMode, Callback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID) + "/play-mode?playMode=" + playMode;

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    PvpClient::s_putPlayModeHolder.spawn(req.put(url), [callback](web::WebResponse res) {
        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void PvpClient::putProgress(int matchID, float progress, bool completed, Callback callback) {
    web::WebRequest req;
    std::string url =
        gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID) + "/progress?progress=" + std::to_string(progress);

    if (completed) {
        url += "&completed=true";
    }

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    PvpClient::s_putProgressHolder.spawn(req.put(url), [callback](web::WebResponse res) {
        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void PvpClient::postDeathProgress(int matchID, float progress, Callback callback) {
    web::WebRequest req;
    std::string url =
        gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID) + "/deaths?progress=" + std::to_string(progress);

    auto body = matjson::Value::object();
    body["progress"] = progress;
    body["count"] = singleDeathCount(progress);
    req.bodyJSON(body);
    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    auto holder = std::make_shared<async::TaskHolder<web::WebResponse>>();
    PvpClient::s_postHolders.push_back(holder);
    holder->spawn(req.post(url), [callback, holder](web::WebResponse res) {
        auto& holders = PvpClient::s_postHolders;
        holders.erase(std::remove(holders.begin(), holders.end(), holder), holders.end());

        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void PvpClient::postDeathCount(int matchID, std::string const& count, Callback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID) + "/deaths?count=" + count;

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    auto holder = std::make_shared<async::TaskHolder<web::WebResponse>>();
    PvpClient::s_postHolders.push_back(holder);
    holder->spawn(req.post(url), [callback, holder](web::WebResponse res) {
        auto& holders = PvpClient::s_postHolders;
        holders.erase(std::remove(holders.begin(), holders.end(), holder), holders.end());

        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void PvpClient::getMessages(int matchID, std::int64_t afterID, int limit, GetMessagesCallback callback) {
    web::WebRequest req;
    auto url = gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID) + "/messages";
    std::vector<std::string> params;

    if (afterID > 0) {
        params.push_back("afterId=" + std::to_string(afterID));
    }

    if (limit > 0) {
        params.push_back("limit=" + std::to_string(limit));
    }

    if (!params.empty()) {
        url += "?";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) {
                url += "&";
            }
            url += params[i];
        }
    }

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    PvpClient::s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        PvpMessagesResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = PvpMessagesResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}

void PvpClient::postMessage(int matchID, std::string const& content, PostMessageCallback callback) {
    web::WebRequest req;
    auto body = matjson::Value::object();
    body["content"] = content;
    req.bodyJSON(body);
    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    auto url = gdvn::config::API_URL + "/pvp/matches/" + std::to_string(matchID) + "/messages";

    auto holder = std::make_shared<async::TaskHolder<web::WebResponse>>();
    PvpClient::s_postHolders.push_back(holder);
    holder->spawn(req.post(url), [callback, holder](web::WebResponse res) {
        auto& holders = PvpClient::s_postHolders;
        holders.erase(std::remove(holders.begin(), holders.end(), holder), holders.end());

        PvpMessageDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = PvpMessageAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}
