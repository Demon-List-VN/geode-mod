#include "UpdaterClient.hpp"

#include "../../adapters/GithubReleaseResponseAdapter.hpp"

async::TaskHolder<web::WebResponse> UpdaterClient::s_getHolder;

void UpdaterClient::getLatestDownload(Callback callback) {
    web::WebRequest req = web::WebRequest();
    req.userAgent("geode");
    auto url = "https://github.com/Demon-List-VN/geode-mod/releases/latest/download/nampe.gdvn.geode";

    UpdaterClient::s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        EmptyResponseDto dto;
        callback(dto, res);
    });
}

void UpdaterClient::getLatestRelease(GetLatestReleaseCallback callback) {
    web::WebRequest req = web::WebRequest();
    req.userAgent("geode");
    auto url = "https://api.github.com/repos/Demon-List-VN/geode-mod/releases/latest";

    UpdaterClient::s_getHolder.spawn(req.get(url), [callback](web::WebResponse res) {
        GithubReleaseResponseDto dto;

        if (res.ok()) {
            auto jsonResult = res.json();
            if (jsonResult) {
                dto = GithubReleaseResponseAdapter::fromJson(jsonResult.unwrap());
            }
        }

        callback(dto, res);
    });
}
