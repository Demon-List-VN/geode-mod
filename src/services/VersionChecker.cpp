#include "VersionChecker.hpp"
#include <Geode/loader/Dirs.hpp>
#include <Geode/loader/ModMetadata.hpp>
#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/ui/Popup.hpp>

async::TaskHolder<web::WebResponse> VersionChecker::m_holder;

namespace {
	constexpr char const* UPDATE_URL = "https://github.com/Demon-List-VN/geode-mod/releases/latest/download/nampe.gdvn.geode";
	constexpr char const* MOD_ID = "nampe.gdvn";
	constexpr char const* MOD_FILE_NAME = "nampe.gdvn.geode";

	void showUpdateToast(std::string const& message, geode::NotificationIcon icon, float time = 2.0f) {
		geode::Loader::get()->queueInMainThread([message, icon, time] {
			geode::Notification::create(message, icon, time)->show();
		});
	}
}

void VersionChecker::downloadUpdate() {
	auto loadingToast = geode::Notification::create(
		"Downloading GDVN update...",
		geode::NotificationIcon::Loading,
		10.0f
	);
	loadingToast->show();

	web::WebRequest req = web::WebRequest();
	req.userAgent("geode");

	m_holder.spawn(req.get(UPDATE_URL), [loadingToast](web::WebResponse res) {
		geode::Loader::get()->queueInMainThread([loadingToast] {
			loadingToast->hide();
		});

		try {
			if (!res.ok()) {
				log::warn("Failed to download GDVN update: HTTP {}", res.code());
				showUpdateToast("Failed to download GDVN update", geode::NotificationIcon::Error);
				return;
			}

			auto tmpPath = dirs::getTempDir() / MOD_FILE_NAME;
			tmpPath += ".tmp";

			auto data = std::move(res).data();
			auto writeTmp = utils::file::writeBinary(tmpPath, data);
			if (!writeTmp) {
				log::warn("Failed to write GDVN update: {}", std::move(writeTmp).unwrapErr());
				showUpdateToast("Failed to save GDVN update", geode::NotificationIcon::Error);
				return;
			}

			auto metadata = ModMetadata::createFromGeodeFile(tmpPath);
			if (metadata.hasErrors() || metadata.getID() != MOD_ID) {
				for (auto const& error : metadata.getErrors()) {
					log::warn("Downloaded GDVN update is invalid: {}", error);
				}

				std::error_code ec;
				std::filesystem::remove(tmpPath, ec);

				showUpdateToast("Downloaded GDVN update is invalid", geode::NotificationIcon::Error);
				return;
			}

			auto targetPath = dirs::getModsDir() / MOD_FILE_NAME;
			auto installedPath = Mod::get()->getPackagePath();

			auto removeExisting = [&tmpPath](std::filesystem::path const& path) -> bool {
				std::error_code ec;
				std::filesystem::remove(path, ec);
				auto stillExists = std::filesystem::exists(path, ec);
				if (ec || stillExists) {
					log::warn("Failed to replace GDVN update at {}: {}", path, ec.message());
					std::filesystem::remove(tmpPath, ec);
					showUpdateToast("Failed to replace GDVN update", geode::NotificationIcon::Error);
					return false;
				}

				return true;
			};

			if (!installedPath.empty() && installedPath != targetPath && !removeExisting(installedPath)) {
				return;
			}

			if (!removeExisting(targetPath)) {
				return;
			}

			std::error_code ec;
			std::filesystem::rename(tmpPath, targetPath, ec);
			if (ec) {
				log::warn("Failed to install GDVN update: {}", ec.message());
				std::filesystem::remove(tmpPath, ec);
				showUpdateToast("Failed to install GDVN update", geode::NotificationIcon::Error);
				return;
			}

			geode::Loader::get()->queueInMainThread([] {
				FLAlertLayer::create(
					"Update Installed",
					"GDVN has been updated.\nPlease restart Geometry Dash to apply the update.",
					"OK"
				)->show();
			});
		} catch (...) {
			log::warn("Failed to install GDVN update");
			showUpdateToast("Failed to install GDVN update", geode::NotificationIcon::Error);
		}
	});
}

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

		    geode::Loader::get()->queueInMainThread([localVersion, latestVersion] {
		        geode::createQuickPopup(
                    "Update Available",
                    "A new version of <cy>Geometry Dash VN</c> is available!\n\nCurrent: <cr>" + localVersion + "</c>\nLatest: <cg>" + latestVersion + "</c>",
                    "Close",
                    "Update",
                    [](auto, bool btn2) {
                        if (btn2) {
                            VersionChecker::downloadUpdate();
                        }
                    }
                );
		    });
		} catch (...) {
			log::warn("Failed to check for updates");
		}
	});
}
