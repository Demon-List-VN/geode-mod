#include "UpdateClient.hpp"

#include "../../adapters/GithubReleaseResponseAdapter.hpp"

namespace {
async::TaskHolder<web::WebResponse> s_getHolder;
}

void UpdateClient::getLatestDownload(Callback callback) {
	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");
	auto url = "https://github.com/Demon-List-VN/geode-mod/releases/latest/download/nampe.gdvn.geode";

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		EmptyResponseDto dto;
		callback(dto, res);
	});
}

void UpdateClient::getLatestRelease(GetLatestReleaseCallback callback) {
	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");
	auto url = "https://api.github.com/repos/Demon-List-VN/geode-mod/releases/latest";

	s_getHolder.spawn(req.get(url), [&](web::WebResponse res) {
		GithubReleaseResponseDto dto;

		if (res.ok()) {
			auto jsonResult = res.json();
			if (jsonResult) {
				dto = gdvn::adapters::GithubReleaseResponseAdapter::fromJson(jsonResult.unwrap());
			}
		}

		callback(dto, res);
	});
}
