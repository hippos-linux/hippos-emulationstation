#pragma once

#include <memory>
#include <string>

#include "HttpReq.h"

class FileData;

enum class ChallengeAnswer
{
	NONE,
	DECLINED,
	CHALLENGER_LEFT
};

class NetPlayChallenge
{
public:
	NetPlayChallenge(FileData* game, bool autoAccept);
	~NetPlayChallenge();

	void update(int deltaTime);

	FileData* getGame() const { return mGame; }
	const std::string& getRoomId() const { return mRoomId; }
	const std::string& getChallengerIp() const { return mChallengerIp; }
	bool hasFailed() const { return mFailed; }
	bool acceptsAutomatically() const { return mAutoAccept; }

	std::string takeNewChallenger();
	void answer(bool accept);
	ChallengeAnswer takeAnswerResult();

private:
	void readCreatedRoom(const std::string& json);
	void readRoomStatus(const std::string& json);
	std::unique_ptr<HttpReq> roomRequest(const std::string& method, const std::string& path = "", const std::string& body = "");
	void finishAnswer(HttpReq::Status status);

	FileData* mGame;
	bool mAutoAccept;
	std::unique_ptr<HttpReq> mRequest;
	std::unique_ptr<HttpReq> mHeartbeat;
	std::unique_ptr<HttpReq> mAnswer;
	std::string mRoomId;
	std::string mToken;
	std::string mChallengerIp;
	std::string mPendingName;
	std::string mPendingIp;
	bool mPendingAnnounced;
	bool mAccepting;
	ChallengeAnswer mAnswerResult;
	int mPollDelay;
	int mHeartbeatDelay;
	bool mFailed;
};
