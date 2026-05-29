#include "EventClient.hpp"

#include "../common.hpp"

async::TaskHolder<web::WebResponse> EventClient::m_put_holder;

void EventClient::submitLevel(int levelID, float progress, std::string const& token, Callback callback) {
	web::WebRequest req;
	std::string url = API_URL + "/events/submitLevel/" + std::to_string(levelID) + "?progress=" + std::to_string(progress) + "&password=" + EVENT_PASSWORD;

	req.header("Authorization", "Bearer " + token);

	m_put_holder.spawn(req.put(url), [&](web::WebResponse res) {
		callback(res);
	});
}
