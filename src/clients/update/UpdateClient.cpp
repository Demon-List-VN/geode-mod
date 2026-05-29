#include "UpdateClient.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
}

void UpdateClient::getLatestDownload(Callback callback) {
	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");
	auto url = "https://github.com/Demon-List-VN/geode-mod/releases/latest/download/nampe.gdvn.geode";

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}

void UpdateClient::getLatestRelease(Callback callback) {
	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");
	auto url = "https://api.github.com/repos/Demon-List-VN/geode-mod/releases/latest";

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		callback(res);
	});
}
