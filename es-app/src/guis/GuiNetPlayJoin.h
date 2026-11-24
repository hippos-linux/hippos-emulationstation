#pragma once

#include "guis/GuiMsgBox.h"
#include "guis/GuiNetPlay.h"

#include <memory>

class HttpReq;

class GuiNetPlayJoin : public GuiMsgBox
{
public:
	GuiNetPlayJoin(Window* window, const LobbyAppEntry& entry);
	~GuiNetPlayJoin();

	void update(int deltaTime) override;

private:
	std::string challengeUrl() const;
	void sendChallengeRequest(const std::string& method);
	void readChallenge(const std::string& json);
	void readChallengeState(const std::string& json);
	void closeWithMessage(const std::string& message);

	LobbyAppEntry mEntry;
	std::unique_ptr<HttpReq> mRequest;
	std::string mToken;
	int mPollDelay;
	bool mClosing;
};
