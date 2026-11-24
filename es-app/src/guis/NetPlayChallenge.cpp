#include "guis/NetPlayChallenge.h"

#include "FileData.h"
#include "HipposLobby.h"

#include <rapidjson/document.h>
#include <SDL_timer.h>

static const int STATUS_POLL_MS = 2000;
static const int HEARTBEAT_MS = 30000;
static const Uint32 REMOVE_ROOM_TIMEOUT_MS = 3000;

NetPlayChallenge::NetPlayChallenge(FileData* game, bool autoAccept)
	: mGame(game),
	mAutoAccept(autoAccept),
	mPendingAnnounced(false),
	mAccepting(false),
	mAnswerResult(ChallengeAnswer::NONE),
	mPollDelay(STATUS_POLL_MS),
	mHeartbeatDelay(HEARTBEAT_MS),
	mFailed(false)
{
	HttpReqOptions options;
	options.dataToPost = HipposLobby::challengeRoomJson(game);
	options.customHeaders.push_back("Content-Type: application/json");
	mRequest = std::unique_ptr<HttpReq>(new HttpReq(HipposLobby::url("/rooms"), &options));
}

NetPlayChallenge::~NetPlayChallenge()
{
	if (mRoomId.empty() || !mChallengerIp.empty())
		return;

	mRequest = roomRequest("DELETE");
	Uint32 deadline = SDL_GetTicks() + REMOVE_ROOM_TIMEOUT_MS;
	while (mRequest->status() == HttpReq::REQ_IN_PROGRESS && SDL_GetTicks() < deadline)
		SDL_Delay(10);
}

std::unique_ptr<HttpReq> NetPlayChallenge::roomRequest(const std::string& method, const std::string& path, const std::string& body)
{
	HttpReqOptions options;
	options.method = method;
	options.customHeaders.push_back("Authorization: Bearer " + mToken);
	if (!body.empty())
	{
		options.dataToPost = body;
		options.customHeaders.push_back("Content-Type: application/json");
	}
	return std::unique_ptr<HttpReq>(new HttpReq(HipposLobby::url("/rooms/" + mRoomId + path), &options));
}

std::string NetPlayChallenge::takeNewChallenger()
{
	if (mPendingName.empty() || mPendingAnnounced)
		return "";

	mPendingAnnounced = true;
	return mPendingName;
}

void NetPlayChallenge::answer(bool accept)
{
	mAccepting = accept;
	mAnswer = roomRequest("POST", "/answer", accept ? "{\"accept\": true}" : "{\"accept\": false}");
}

ChallengeAnswer NetPlayChallenge::takeAnswerResult()
{
	ChallengeAnswer result = mAnswerResult;
	mAnswerResult = ChallengeAnswer::NONE;
	return result;
}

void NetPlayChallenge::finishAnswer(HttpReq::Status status)
{
	if (status != HttpReq::REQ_SUCCESS)
		mAnswerResult = ChallengeAnswer::CHALLENGER_LEFT;
	else if (mAccepting)
		mChallengerIp = mPendingIp;
	else
		mAnswerResult = ChallengeAnswer::DECLINED;

	mPendingName.clear();
	mPendingIp.clear();
}

void NetPlayChallenge::update(int deltaTime)
{
	if (mFailed || !mChallengerIp.empty())
		return;

	if (mHeartbeat && mHeartbeat->status() != HttpReq::REQ_IN_PROGRESS)
		mHeartbeat.reset();

	if (mAnswer && mAnswer->status() != HttpReq::REQ_IN_PROGRESS)
	{
		finishAnswer(mAnswer->status());
		mAnswer.reset();
	}

	if (!mRoomId.empty())
	{
		mHeartbeatDelay -= deltaTime;
		if (mHeartbeatDelay <= 0 && !mHeartbeat)
		{
			mHeartbeatDelay = HEARTBEAT_MS;
			mHeartbeat = roomRequest("PUT");
		}
	}

	if (mRequest)
	{
		auto status = mRequest->status();
		if (status == HttpReq::REQ_IN_PROGRESS)
			return;

		std::string content = status == HttpReq::REQ_SUCCESS ? mRequest->getContent() : "";
		mRequest.reset();

		if (status != HttpReq::REQ_SUCCESS)
			mFailed = true;
		else if (mRoomId.empty())
			readCreatedRoom(content);
		else if (!mAnswer)
			readRoomStatus(content);
		return;
	}

	mPollDelay -= deltaTime;
	if (mPollDelay <= 0 && !mRoomId.empty())
	{
		mPollDelay = STATUS_POLL_MS;
		mRequest = roomRequest("GET");
	}
}

void NetPlayChallenge::readCreatedRoom(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("id") || !doc["id"].IsString() || !doc.HasMember("token") || !doc["token"].IsString())
	{
		mFailed = true;
		return;
	}

	mRoomId = doc["id"].GetString();
	mToken = doc["token"].GetString();
}

void NetPlayChallenge::readRoomStatus(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("challenger"))
		return;

	const auto& challenger = doc["challenger"];
	if (!challenger.IsObject() || !challenger.HasMember("username") || !challenger["username"].IsString() ||
		!challenger.HasMember("ip") || !challenger["ip"].IsString())
	{
		mPendingName.clear();
		mPendingIp.clear();
		return;
	}

	std::string username = challenger["username"].GetString();
	if (username != mPendingName)
	{
		mPendingName = username;
		mPendingIp = challenger["ip"].GetString();
		mPendingAnnounced = false;
	}
}
