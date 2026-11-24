#include "guis/GuiNetPlayChallenge.h"

#include "FileData.h"
#include "HipposLobby.h"
#include "HttpReq.h"
#include "LocaleES.h"
#include "Window.h"
#include "views/ViewController.h"

#include <rapidjson/document.h>
#include <SDL_timer.h>

static const int STATUS_POLL_MS = 2000;
static const int HEARTBEAT_MS = 30000;
static const Uint32 REMOVE_ROOM_TIMEOUT_MS = 3000;

GuiNetPlayChallenge::GuiNetPlayChallenge(Window* window, FileData* game, const std::function<void()>& onMatchStarted)
	: GuiSettings(window, _("WAITING FOR A CHALLENGER")),
	mGame(game),
	mOnMatchStarted(onMatchStarted),
	mPollDelay(STATUS_POLL_MS),
	mHeartbeatDelay(HEARTBEAT_MS),
	mMatched(false)
{
	setSubTitle(game->getName());
	addEntry(_("CANCEL"), false, [this] { close(); });

	HttpReqOptions options;
	options.dataToPost = HipposLobby::challengeRoomJson(game);
	options.customHeaders.push_back("Content-Type: application/json");
	mRequest = std::unique_ptr<HttpReq>(new HttpReq(HipposLobby::url("/rooms"), &options));
}

GuiNetPlayChallenge::~GuiNetPlayChallenge()
{
	if (!mMatched)
		removeRoom();
}

void GuiNetPlayChallenge::sendRoomRequest(const std::string& method)
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

void GuiNetPlayChallenge::removeRoom()
{
	if (mRoomId.empty())
		return;

	sendRoomRequest("DELETE");
	Uint32 deadline = SDL_GetTicks() + REMOVE_ROOM_TIMEOUT_MS;
	while (mRequest->status() == HttpReq::REQ_IN_PROGRESS && SDL_GetTicks() < deadline)
		SDL_Delay(10);
}

void GuiNetPlayChallenge::update(int deltaTime)
{
	GuiSettings::update(deltaTime);

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
		{
			mWindow->displayNotificationMessage(_("THE NETPLAY LOBBY CAN'T BE REACHED"));
			close();
			return;
		}

		if (mRoomId.empty())
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

void GuiNetPlayChallenge::readCreatedRoom(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("id") || !doc["id"].IsString() || !doc.HasMember("token") || !doc["token"].IsString())
	{
		close();
		return;
	}

	mRoomId = doc["id"].GetString();
	mToken = doc["token"].GetString();
}

void GuiNetPlayChallenge::readRoomStatus(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("challenger") || !doc["challenger"].IsObject())
		return;

	const auto& challenger = doc["challenger"];
	if (!challenger.HasMember("ip") || !challenger["ip"].IsString())
		return;

	mMatched = true;

	LaunchGameOptions options;
	options.netPlayMode = SERVER;
	options.ip = challenger["ip"].GetString();
	ViewController::get()->launch(mGame, options);

	auto onMatchStarted = mOnMatchStarted;
	close();
	onMatchStarted();
}
