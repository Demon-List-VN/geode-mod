#include "OTPLogin.hpp"
#include "../common.hpp"

async::TaskHolder<web::WebResponse> OTPLogin::m_post_holder, OTPLogin::m_get_holder;

bool OTPLogin::isLoggedIn() {
	std::string apiKey = Mod::get()->getSettingValue<std::string>("api-key");
	return !apiKey.empty();
}

void OTPLogin::showLoginPrompt() {
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
}

void OTPLogin::requestOTP() {
	web::WebRequest req;

	m_post_holder.spawn(req.post(API_URL + "/auth/otp"), [](web::WebResponse res) {
		try {
			if (!res.ok()) {
				log::warn("Failed to create OTP code: HTTP {}", res.code());
				FLAlertLayer::create("Error", "Failed to create login code. Please try again.", "OK")->show();
				return;
			}

			auto json = res.json().unwrap();
			int code = json["code"].asInt().unwrap();

			std::string url = "https://www.gdvn.net/auth/otp/" + std::to_string(code);
			web::openLinkInBrowser(url);

			geode::createQuickPopup(
				"GDVN Login",
				"A browser window has been opened.\nPlease <cy>grant access</c> on the website, then click <cg>Continue</c>.",
				"Cancel",
				"Continue",
				[code](auto, bool btn2) {
					if (btn2) {
						checkOTP(code);
					}
				}
			);
		} catch (...) {
			log::warn("Failed to create OTP code: unexpected error");
			FLAlertLayer::create("Error", "Failed to create login code. Please try again.", "OK")->show();
		}
	});
}

void OTPLogin::checkOTP(int code) {
	web::WebRequest req;
	std::string url = API_URL + "/auth/otp/" + std::to_string(code);

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

			Mod::get()->setSettingValue("api-key", key);

			FLAlertLayer::create("GDVN Login", "You are logged in as <cg>" + player + "</c>", "OK")->show();
		} catch (...) {
			log::warn("Failed to verify OTP code: unexpected error");
			FLAlertLayer::create("Error", "Failed to verify login code. Please try again.", "OK")->show();
		}
	});
}

void OTPLogin::logout() {
	Mod::get()->setSettingValue("api-key", std::string(""));
	FLAlertLayer::create("GDVN", "You have been logged out.", "OK")->show();
}
