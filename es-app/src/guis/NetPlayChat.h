#pragma once

#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/TextComponent.h"

#include <memory>
#include <string>

class HttpReq;

class NetPlayChat : public ComponentGrid
{
public:
	static const std::string GLOBAL_CHANNEL;

	NetPlayChat(Window* window);
	~NetPlayChat();

	void setChannel(const std::string& channel, const std::string& label);
	const std::string& getChannel() const { return mChannel; }
	void say();

	void update(int deltaTime) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	std::string chatUrl() const;
	void startPoll();
	void readMessages(const std::string& json);
	void addMessage(const std::string& username, const std::string& text);
	void sendMessage(const std::string& text);
	void finishSend();
	void confirmMute(const std::string& username);

	std::shared_ptr<TextComponent> mChannelLabel;
	std::shared_ptr<ComponentList> mMessages;
	std::unique_ptr<HttpReq> mPollRequest;
	std::unique_ptr<HttpReq> mSendRequest;
	std::string mChannel;
	int mLastMessageId;
	int mPollDelay;
};
