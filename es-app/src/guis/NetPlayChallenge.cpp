#include "guis/NetPlayChallenge.h"

#include "FileData.h"
#include "HipposLobby.h"
#include "HttpReq.h"

#include <rapidjson/document.h>
#include <SDL_timer.h>

static const int STATUS_POLL_MS = 2000;
static const int HEARTBEAT_MS = 30000;
static const Uint32 REMOVE_ROOM_TIMEOUT_MS = 3000;

NetPlayChallenge::NetPlayChallenge(FileData* game)
	: mGame(game),
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

	sendRoomRequest("DELETE");
	Uint32 deadline = SDL_GetTicks() + REMOVE_ROOM_TIMEOUT_MS;
	while (mRequest->status() == HttpReq::REQ_IN_PROGRESS && SDL_GetTicks() < deadline)
		SDL_Delay(10);
}

void NetPlayChallenge::sendRoomRequest(const std::string& method)
{
	HttpReqOptions options;
	options.method = method;
	options.customHeaders.push_back("Authorization: Bearer " + mToken);
	auto request = std::unique_ptr<HttpReq>(new HttpReq(HipposLobby::url("/rooms/" + mRoomId), &options));

	if (method == "PUT")
		mHeartbeat = std::move(request);
	else
		mRequest = std::move(request);
}

void NetPlayChallenge::update(int deltaTime)
{
	if (mFailed || !mChallengerIp.empty())
		return;

	if (mHeartbeat && mHeartbeat->status() != HttpReq::REQ_IN_PROGRESS)
		mHeartbeat.reset();

	if (!mRoomId.empty())
	{
		mHeartbeatDelay -= deltaTime;
		if (mHeartbeatDelay <= 0 && !mHeartbeat)
		{
			mHeartbeatDelay = HEARTBEAT_MS;
			sendRoomRequest("PUT");
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
		else
			readRoomStatus(content);
		return;
	}

	mPollDelay -= deltaTime;
	if (mPollDelay <= 0 && !mRoomId.empty())
	{
		mPollDelay = STATUS_POLL_MS;
		sendRoomRequest("GET");
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
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("challenger") || !doc["challenger"].IsObject())
		return;

	const auto& challenger = doc["challenger"];
	if (challenger.HasMember("ip") && challenger["ip"].IsString())
		mChallengerIp = challenger["ip"].GetString();
}
