#include "VersionCheckerService.hpp"
#include "../../clients/update/UpdateClient.hpp"
#include <Geode/loader/Dirs.hpp>
#include <Geode/loader/ModMetadata.hpp>
#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/ui/Popup.hpp>

static void showUpdateToast(std::string const& message, geode::NotificationIcon icon, float time = 2.0f) {
	geode::Loader::get()->queueInMainThread([&] {
		geode::Notification::create(message, icon, time)->show();
	});
}

void VersionCheckerService::downloadUpdate() {
	auto loadingToast = geode::Notification::create(
		"Downloading GDVN update...",
		geode::NotificationIcon::Loading,
		10.0f
	);
	loadingToast->show();

	UpdateClient::getLatestDownload([&](EmptyResponseDto const&, web::WebResponse& res) {
		geode::Loader::get()->queueInMainThread([&] {
			loadingToast->hide();
		});

		if (!res.ok()) {
			log::warn("Failed to download GDVN update: HTTP {}", res.code());
			showUpdateToast("Failed to download GDVN update", geode::NotificationIcon::Error);
			return;
		}

		auto tmpPath = dirs::getTempDir() / "nampe.gdvn.geode";
		tmpPath += ".tmp";

		auto data = std::move(res).data();
		auto writeTmp = utils::file::writeBinary(tmpPath, data);
		if (!writeTmp) {
			log::warn("Failed to write GDVN update: {}", std::move(writeTmp).unwrapErr());
			showUpdateToast("Failed to save GDVN update", geode::NotificationIcon::Error);
			return;
		}

		auto metadata = ModMetadata::createFromGeodeFile(tmpPath);
		if (metadata.hasErrors() || metadata.getID() != "nampe.gdvn") {
			for (auto const& error : metadata.getErrors()) {
				log::warn("Downloaded GDVN update is invalid: {}", error);
			}

			std::error_code ec;
			std::filesystem::remove(tmpPath, ec);

			showUpdateToast("Downloaded GDVN update is invalid", geode::NotificationIcon::Error);
			return;
		}

		auto targetPath = dirs::getModsDir() / "nampe.gdvn.geode";
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

		geode::Loader::get()->queueInMainThread([&] {
			FLAlertLayer::create(
				"Update Installed",
				"GDVN has been updated.\nPlease restart Geometry Dash to apply the update.",
				"OK"
			)->show();
		});
	});
}

void VersionCheckerService::checkForUpdate(bool notifyIfCurrent) {
	UpdateClient::getLatestRelease([&](GithubReleaseResponseDto const& release, web::WebResponse& res) {
		if (!res.ok()) {
			if (notifyIfCurrent) {
				showUpdateToast("Failed to check for GDVN updates", geode::NotificationIcon::Error);
			}
			return;
		}

		if (!release.valid) {
			return;
		}

		std::string latestVersion = release.tagName;
		std::string localVersion = Mod::get()->getVersion().toNonVString();

		if (latestVersion == localVersion) {
			if (notifyIfCurrent) {
				showUpdateToast("GDVN is already up to date", geode::NotificationIcon::Success);
			}
			return;
		}

		geode::Loader::get()->queueInMainThread([&] {
			geode::createQuickPopup(
				"Update Available",
				"A new version of <cy>Geometry Dash VN</c> is available!\n\nCurrent: <cr>" + localVersion + "</c>\nLatest: <cg>" + latestVersion + "</c>",
				"Close",
				"Update",
				[&](auto, bool btn2) {
					if (btn2) {
						VersionCheckerService::downloadUpdate();
					}
				}
			);
		});
	});
}
