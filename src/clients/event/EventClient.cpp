#include "EventClient.hpp"

#include "../../common.hpp"
#include "../../utils/AuthConfig.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
async::TaskHolder<web::WebResponse> s_putHolder;
}

void EventClient::getEventLevel(int levelID, std::string const& type, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/levels/" + std::to_string(levelID) + "/inEvent";

	if (!type.empty()) {
		url += "?type=" + type;
	}

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void EventClient::putLevel(int levelID, float progress, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(progress) + "&password=" + EVENT_PASSWORD;

	req.header("Authorization", "Bearer " + gdvn::auth_config::getToken());

	s_putHolder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}
