#include "EventClient.hpp"

#include "../common.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_putHolder;
}

static std::string getToken() {
	return Mod::get()->getSavedValue<std::string>("api-key");
}

void EventClient::putLevel(int levelID, float progress, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(progress) + "&password=" + EVENT_PASSWORD;

	req.header("Authorization", "Bearer " + getToken());

	s_putHolder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}
