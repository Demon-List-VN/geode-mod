#include "../auth/AuthService.hpp"
#include "../../clients/auth/AuthClient.hpp"
#include "../../config.hpp"
#include <Geode/ui/Notification.hpp>

static void showOTPDialog(std::string const& code) {
    auto grantUrl = gdvn::config::WEBSITE_URL + "/auth/otp/" + code;

    geode::createQuickPopup(
        "GDVN Login",
        "Open GDVN website to grant access, then click <cg>Continue</c>.\nIf the website does not open, open GDVN "
        "website manually and go to <cy>Settings > Auth > Grant OTP</c>.\nYour OTP code is: <cr>" +
            code + "</c>",
        "Open Website", "Continue", [code, grantUrl](auto, bool btn2) {
            if (btn2) {
                AuthService::checkOTP(code);
            } else {
                web::openLinkInBrowser(grantUrl);
                showOTPDialog(code);
            }
        });
}

bool AuthService::isLoggedIn() {
    std::string apiKey = Mod::get()->getSavedValue<std::string>("api-key");
    return !apiKey.empty();
}

std::string AuthService::getPlayerName() {
    return Mod::get()->getSavedValue<std::string>("player-name");
}

void AuthService::login() {
    geode::Loader::get()->queueInMainThread([] {
        geode::createQuickPopup("GDVN Login", "Do you want to <cy>login</c> to Geometry Dash VN?", "Later", "Login",
                                [](auto, bool btn2) {
                                    if (btn2) {
                                        requestOTP();
                                    }
                                });
    });
}

void AuthService::requestOTP() {
    AuthClient::postOTP([=](OtpResponseDto const& otp, web::WebResponse& res) {
        if (!res.ok()) {
            log::warn("Failed to create OTP code: HTTP {}", res.code());
            FLAlertLayer::create("Error", "Failed to create login code. Please try again.", "OK")->show();
            return;
        }

        if (!otp.valid) {
            log::warn("Failed to create OTP code: invalid response");
            FLAlertLayer::create("Error", "Failed to create login code. Please try again.", "OK")->show();
            return;
        }

        auto code = otp.code;

        geode::Loader::get()->queueInMainThread([code] { showOTPDialog(code); });
    });
}

void AuthService::checkOTP(std::string code) {
    AuthClient::getOTP(code, [=](OtpGrantResponseDto const& grant, web::WebResponse& res) {
        if (!res.ok()) {
            log::warn("Failed to verify OTP code: HTTP {}", res.code());
            FLAlertLayer::create("Error", "Failed to verify login code. Please try again.", "OK")->show();
            return;
        }

        if (!grant.granted) {
            FLAlertLayer::create("GDVN Login",
                                 "Access has not been granted yet.\nPlease grant access on the website first.", "OK")
                ->show();
            return;
        }

        if (!grant.valid) {
            log::warn("Failed to verify OTP code: missing credentials");
            FLAlertLayer::create("Error", "Failed to verify login code. Please try again.", "OK")->show();
            return;
        }

        Mod::get()->setSavedValue("api-key", grant.key);
        Mod::get()->setSavedValue("player-name", grant.player);

        FLAlertLayer::create("GDVN Login", "You are logged in as <cg>" + grant.player + "</c>", "OK")->show();
    });
}

void AuthService::logout() {
    AuthClient::deleteAPIKey([=](EmptyResponseDto const&, web::WebResponse& res) {
        Mod::get()->setSavedValue("api-key", std::string(""));
        Mod::get()->setSavedValue("player-name", std::string(""));
        FLAlertLayer::create("GDVN", "You have been logged out.", "OK")->show();
    });
}

void AuthService::check() {
    auto loadingToast =
        geode::Notification::create("Checking GDVN login status...", geode::NotificationIcon::Loading, 10.0f);

    loadingToast->show();

    if (!isLoggedIn()) {
        loadingToast->hide();

        auto errorToast = geode::Notification::create("Not logged in to GDVN", geode::NotificationIcon::Error, 2.0f);

        errorToast->show();

        return;
    }

    AuthClient::getMe([loadingToast](AuthMeResponseDto const& authMe, web::WebResponse& res) {
        auto ok = res.ok();
        auto code = res.code();
        auto valid = authMe.valid;
        auto name = authMe.name;

        geode::Loader::get()->queueInMainThread([loadingToast, ok, code, valid, name] {
            loadingToast->hide();

            if (!ok) {
                if (code == 426) {
                    geode::Notification::create("Please update GDVN mod", geode::NotificationIcon::Error, 2.0f)->show();

                    return;
                }

                Mod::get()->setSavedValue("api-key", std::string(""));
                Mod::get()->setSavedValue("player-name", std::string(""));

                geode::Notification::create("Token expired. Please log back in", geode::NotificationIcon::Error, 2.0f)
                    ->show();

                return;
            }

            if (!valid) {
                log::warn("Failed to check login status: invalid response");
                return;
            }

            Mod::get()->setSavedValue("player-name", name);

            geode::Notification::create("Logged in as " + name, geode::NotificationIcon::Success, 2.0f)->show();
        });
    });
}
