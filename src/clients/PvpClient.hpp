#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <cstdint>
#include <functional>
#include <string>

using namespace geode::prelude;

class PvpClient {
	static async::TaskHolder<web::WebResponse> m_put_holder;
	static async::TaskHolder<web::WebResponse> m_post_holder;
	static async::TaskHolder<web::WebResponse> m_get_holder;

public:
	using Callback = std::function<void(web::WebResponse&)>;

	static void submitPlayMode(int matchID, std::string const& playMode, std::string const& token, Callback callback);
	static void submitProgress(int matchID, float progress, bool completed, std::string const& token, Callback callback);
	static void submitDeathCount(int matchID, std::string const& count, std::string const& token, Callback callback);
	static void getMessages(int matchID, std::int64_t afterID, int limit, std::string const& token, Callback callback);
	static void sendMessage(int matchID, std::string const& content, std::string const& token, Callback callback);
};
