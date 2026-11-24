#pragma once

#include "guis/GuiSettings.h"

#include <functional>
#include <memory>
#include <string>

class FileData;
class HttpReq;

class GuiNetPlayChallenge : public GuiSettings
{
public:
	GuiNetPlayChallenge(Window* window, FileData* game, const std::function<void()>& onMatchStarted);
	~GuiNetPlayChallenge();

	void update(int deltaTime) override;

private:
	void readCreatedRoom(const std::string& json);
	void readRoomStatus(const std::string& json);
	void sendRoomRequest(const std::string& method);
	void removeRoom();

	FileData* mGame;
	std::function<void()> mOnMatchStarted;
	std::unique_ptr<HttpReq> mRequest;
	std::unique_ptr<HttpReq> mHeartbeat;
	std::string mRoomId;
	std::string mToken;
	int mPollDelay;
	int mHeartbeatDelay;
	bool mMatched;
};
