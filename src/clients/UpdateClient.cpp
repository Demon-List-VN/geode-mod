#include "UpdateClient.hpp"

async::TaskHolder<web::WebResponse> UpdateClient::m_get_holder;

namespace {
constexpr char const* UPDATE_URL = "https://github.com/Demon-List-VN/geode-mod/releases/latest/download/nampe.gdvn.geode";
constexpr char const* RELEASE_URL = "https://api.github.com/repos/Demon-List-VN/geode-mod/releases/latest";
}

void UpdateClient::downloadLatest(Callback callback) {
	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");

	m_get_holder.spawn(req.get(UPDATE_URL), [&](web::WebResponse res) {
		callback(res);
	});
}

void UpdateClient::getLatestRelease(Callback callback) {
	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");

	m_get_holder.spawn(req.get(RELEASE_URL), [&](web::WebResponse res) {
		callback(res);
	});
}
