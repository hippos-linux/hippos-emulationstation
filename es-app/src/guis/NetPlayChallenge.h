#pragma once

#include <memory>
#include <string>

class FileData;
class HttpReq;

class NetPlayChallenge
{
public:
	NetPlayChallenge(FileData* game);
	~NetPlayChallenge();

	void update(int deltaTime);

	FileData* getGame() const { return mGame; }
	const std::string& getRoomId() const { return mRoomId; }
	const std::string& getChallengerIp() const { return mChallengerIp; }
	bool hasFailed() const { return mFailed; }

private:
	void readCreatedRoom(const std::string& json);
	void readRoomStatus(const std::string& json);
	void sendRoomRequest(const std::string& method);

	FileData* mGame;
	std::unique_ptr<HttpReq> mRequest;
	std::unique_ptr<HttpReq> mHeartbeat;
	std::string mRoomId;
	std::string mToken;
	std::string mChallengerIp;
	int mPollDelay;
	int mHeartbeatDelay;
	bool mFailed;
};
