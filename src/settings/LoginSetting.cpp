#include "LoginSetting.hpp"
#include "../services/OTPLogin.hpp"

Result<std::shared_ptr<SettingV3>> LoginSetting::parse(
	std::string const& key, std::string const& modID, matjson::Value const& json
) {
	auto res = std::make_shared<LoginSetting>();
	auto root = checkJson(json, "LoginSetting");
	res->init(key, modID, root);
	res->parseNameAndDescription(root);
	res->parseEnableIf(root);
	root.checkUnknownKeys();
	return root.ok(std::static_pointer_cast<SettingV3>(res));
}

bool LoginSetting::load(matjson::Value const& json) { return true; }
bool LoginSetting::save(matjson::Value& json) const { return true; }
bool LoginSetting::isDefaultValue() const { return true; }
void LoginSetting::reset() {}

SettingNodeV3* LoginSetting::createNode(float width) {
	return LoginSettingNode::create(
		std::static_pointer_cast<LoginSetting>(shared_from_this()), width
	);
}

LoginSettingNode* LoginSettingNode::create(std::shared_ptr<LoginSetting> setting, float width) {
	auto ret = new LoginSettingNode();
	if (ret && ret->init(setting, width)) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

bool LoginSettingNode::init(std::shared_ptr<LoginSetting> setting, float width) {
	if (!SettingNodeV3::init(setting, width))
		return false;

	bool loggedIn = OTPLogin::isLoggedIn();

	m_buttonSprite = ButtonSprite::create(loggedIn ? "Logout" : "Login");
	m_buttonSprite->setScale(0.5f);

	m_button = CCMenuItemSpriteExtra::create(
		m_buttonSprite, this,
		loggedIn ? menu_selector(LoginSettingNode::onLogout) : menu_selector(LoginSettingNode::onLogin)
	);

	this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
	this->getButtonMenu()->setContentWidth(60);
	this->getButtonMenu()->updateLayout();

	this->updateState(nullptr);

	return true;
}

void LoginSettingNode::updateState(CCNode* invoker) {
	SettingNodeV3::updateState(invoker);

	auto shouldEnable = this->getSetting()->shouldEnable();
	m_button->setEnabled(shouldEnable);
	m_buttonSprite->setCascadeColorEnabled(true);
	m_buttonSprite->setCascadeOpacityEnabled(true);
	m_buttonSprite->setOpacity(shouldEnable ? 255 : 155);
	m_buttonSprite->setColor(shouldEnable ? ccWHITE : ccGRAY);
}

void LoginSettingNode::onLogin(CCObject*) {
	OTPLogin::requestOTP();
}

void LoginSettingNode::onLogout(CCObject*) {
	OTPLogin::logout();
}

void LoginSettingNode::onCommit() {}
void LoginSettingNode::onResetToDefault() {}
bool LoginSettingNode::hasUncommittedChanges() const { return false; }
bool LoginSettingNode::hasNonDefaultValue() const { return false; }

$on_mod(Loaded) {
	(void)Mod::get()->registerCustomSettingType("LoginSetting", &LoginSetting::parse);
}
