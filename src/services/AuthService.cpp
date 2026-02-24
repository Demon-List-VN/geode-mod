#include "AuthService.hpp"
#include "../common.hpp"
#include <Geode/ui/Notification.hpp>

async::TaskHolder<web::WebResponse> AuthService::m_post_holder, AuthService::m_get_holder;

bool AuthService::isLoggedIn() {
	std::string apiKey = Mod::get()->getSavedValue<std::string>("api-key");
	return !apiKey.empty();
}

std::string AuthService::getToken() {
    std::string apiKey = Mod::get()->getSavedValue<std::string>("api-key");
    return apiKey;
}

void AuthService::login() {
    geode::Loader::get()->queueInMainThread([] {
        geode::createQuickPopup(
            "GDVN Login",
            "Do you want to <cy>login</c> to Geometry Dash VN?",
            "Later",
            "Login",
            [](auto, bool btn2) {
                if (btn2) {
                    requestOTP();
                }
            }
        );
    });

}

void AuthService::requestOTP() {
	web::WebRequest req;

	m_post_holder.spawn(req.post(API_URL + "/auth/otp"), [](web::WebResponse res) {
		try {
			if (!res.ok()) {
				log::warn("Failed to create OTP code: HTTP {}", res.code());
				FLAlertLayer::create("Error", "Failed to create login code. Please try again.", "OK")->show();
				return;
			}

			auto json = res.json().unwrap();
			auto code = json["code"].asString().unwrap();

			web::openLinkInBrowser("https://www.gdvn.net/auth/otp/" + code);

		    geode::Loader::get()->queueInMainThread([code] {
		        geode::createQuickPopup(
                    "GDVN Login",
                    "A browser window has been opened.\n<cy>Grant access</c> on the website, then click <cg>Continue</c>.\n Your OTP code is: <cr>" + code + "</c>",
                    "Cancel",
                    "Continue",
                    [code](auto, bool btn2) {
                        if (btn2) {
                            checkOTP(code);
                        }
                    }
                );
		    });
		} catch (...) {
			log::warn("Failed to create OTP code: unexpected error");
			FLAlertLayer::create("Error", "Failed to create login code. Please try again.", "OK")->show();
		}
	});
}

void AuthService::checkOTP(std::string code) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/otp/" + code;

	m_get_holder.spawn(req.get(url), [](web::WebResponse res) {
		try {
			if (!res.ok()) {
				log::warn("Failed to verify OTP code: HTTP {}", res.code());
				FLAlertLayer::create("Error", "Failed to verify login code. Please try again.", "OK")->show();
				return;
			}

			auto json = res.json().unwrap();
			bool granted = json["granted"].asBool().unwrap();

			if (!granted) {
				FLAlertLayer::create("GDVN Login", "Access has not been granted yet.\nPlease grant access on the website first.", "OK")->show();
				return;
			}

			std::string key = json["key"].asString().unwrap();
			std::string player = json["player"].asString().unwrap();

			Mod::get()->setSavedValue("api-key", key);

			FLAlertLayer::create("GDVN Login", "You are logged in as <cg>" + player + "</c>", "OK")->show();
		} catch (...) {
			log::warn("Failed to verify OTP code: unexpected error");
			FLAlertLayer::create("Error", "Failed to verify login code. Please try again.", "OK")->show();
		}
	});
}

void AuthService::logout() {
    web::WebRequest req;
    std::string url = API_URL + "/APIKey";

    req.header("Authorization", "Bearer " + getToken());

    m_post_holder.spawn(req.send("DELETE", url), [](web::WebResponse res) {
        try {
            Mod::get()->setSavedValue("api-key", std::string(""));
            FLAlertLayer::create("GDVN", "You have been logged out.", "OK")->show();
        } catch (...) {
            log::warn("Failed to logout: unexpected error");
        }
    });
}

void AuthService::check() {
    web::WebRequest req;
    std::string url = API_URL + "/auth/me";

    auto loadingToast = geode::Notification::create(
        "Checking GDVN login status...",
        geode::NotificationIcon::Loading,
        10.0f
    );

    loadingToast->show();

    if (!isLoggedIn()) {
        loadingToast->hide();

        auto errorToast = geode::Notification::create(
            "Not logged in to GDVN",
            geode::NotificationIcon::Error,
            2.0f
        );

        errorToast->show();

        return;
    }

    req.header("Authorization", "Bearer " + getToken());

    m_get_holder.spawn(req.get(url), [loadingToast](web::WebResponse res) {
        try {
            loadingToast->hide();

            if (!res.ok()) {
                auto errorToast = geode::Notification::create(
                    "Token expired. Please log back in",
                    geode::NotificationIcon::Error,
                    2.0f
                );

                Mod::get()->setSavedValue("api-key", std::string(""));
                errorToast->show();

                return;
            }

            auto successToast = geode::Notification::create(
            "Logged in as " + res.json().unwrap()["name"].asString().unwrap(),
                geode::NotificationIcon::Success,
                2.0f
            );

            successToast->show();
        } catch (...) {
            log::warn("Failed to check: unexpected error");
        }
    });
}
