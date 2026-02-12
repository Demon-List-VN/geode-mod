#include "VersionChecker.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/Popup.hpp>

async::TaskHolder<web::WebResponse> VersionChecker::m_holder;

void VersionChecker::checkForUpdate() {
	web::WebRequest req = web::WebRequest();
    req.userAgent("geode");

	m_holder.spawn(req.get("https://api.github.com/repos/Demon-List-VN/geode-mod/releases/latest"), [](web::WebResponse res) {
		try {
			if (!res.ok()) {
				return;
			}

			auto resJson = res.json().unwrap();

			if (!resJson["tag_name"].isString()) {
				return;
			}

			std::string latestVersion = resJson["tag_name"].asString().unwrap();
			std::string localVersion = Mod::get()->getVersion().toNonVString();

			if (latestVersion == localVersion) {
				return;
			}

			std::string releaseUrl = "https://github.com/Demon-List-VN/geode-mod/releases/latest";

		    geode::Loader::get()->queueInMainThread([localVersion, latestVersion, releaseUrl] {
		        geode::createQuickPopup(
                    "Update Available",
                    "A new version of <cy>Geometry Dash VN</c> is available!\n\nCurrent: <cr>" + localVersion + "</c>\nLatest: <cg>" + latestVersion + "</c>",
                    "Close",
                    "Download",
                    [releaseUrl](auto, bool btn2) {
                        if (btn2) {
                            web::openLinkInBrowser(releaseUrl);
                        }
                    }
                );
		    });
		} catch (...) {
			log::warn("Failed to check for updates");
		}
	});
}
