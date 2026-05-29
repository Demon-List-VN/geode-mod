#include "EventClient.hpp"

#include "../../config.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
async::TaskHolder<web::WebResponse> s_putHolder;
} // namespace

void EventClient::getEventLevel(int levelID, std::string const& type, Callback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/levels/" + std::to_string(levelID) + "/inEvent";

    if (!type.empty()) {
        url += "?type=" + type;
    }

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void EventClient::putLevel(int levelID, float progress, Callback callback) {
    web::WebRequest req;
    std::string url = gdvn::config::API_URL + "/events/submitLevel/" + std::to_string(levelID) +
                      "?progress=" + std::to_string(progress) + "&password=" + gdvn::config::EVENT_PASSWORD;

    req.header("Authorization", "Bearer " + gdvn::config::getToken());

    s_putHolder.spawn(req.put(url), [callback](web::WebResponse res) {
        EmptyResponseDto dto;
        callback(dto, res);
    });
}
