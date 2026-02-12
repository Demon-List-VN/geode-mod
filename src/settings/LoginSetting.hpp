#pragma once

#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

class LoginSetting : public SettingV3 {
public:
	static Result<std::shared_ptr<SettingV3>> parse(
		std::string const& key, std::string const& modID, matjson::Value const& json
	);
	bool load(matjson::Value const& json) override;
	bool save(matjson::Value& json) const override;
	bool isDefaultValue() const override;
	void reset() override;
	SettingNodeV3* createNode(float width) override;
};

class LoginSettingNode : public SettingNodeV3 {
protected:
	void onLogin(CCObject*);
	void onLogout(CCObject*);
	void onCommit() override;
	void onResetToDefault() override;
public:
	bool hasUncommittedChanges() const override;
	bool hasNonDefaultValue() const override;
	static LoginSettingNode* create(std::shared_ptr<LoginSetting> setting, float width);
	bool init(std::shared_ptr<LoginSetting> setting, float width);
};
