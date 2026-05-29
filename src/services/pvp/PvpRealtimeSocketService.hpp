#pragma once

#include <memory>
#include <string>

class PvpRealtimeSocketDelegateService {
public:
	virtual ~PvpRealtimeSocketDelegateService() = default;
	virtual void onRealtimeOpen() = 0;
	virtual void onRealtimeMessage(std::string const& message) = 0;
	virtual void onRealtimeClose() = 0;
};

class PvpRealtimeSocketService {
public:
	virtual ~PvpRealtimeSocketService() = default;

	static std::shared_ptr<PvpRealtimeSocketService> create(PvpRealtimeSocketDelegateService* delegate);

	virtual bool connect(std::string const& url) = 0;
	virtual void send(std::string const& message) = 0;
	virtual void close() = 0;
	virtual bool isOpen() const = 0;
};
