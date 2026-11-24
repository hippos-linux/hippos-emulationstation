#include "guis/GuiNetPlayJoin.h"

#include "FileData.h"
#include "HipposLobby.h"
#include "HttpReq.h"
#include "LocaleES.h"
#include "SystemConf.h"
#include "Window.h"
#include "guis/GuiNetPlayHost.h"
#include "utils/StringUtil.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <SDL_timer.h>

static const int STATE_POLL_MS = 2000;
static const Uint32 CANCEL_TIMEOUT_MS = 3000;

GuiNetPlayJoin::GuiNetPlayJoin(Window* window, const LobbyAppEntry& entry)
	: GuiMsgBox(window, Utils::String::format(_("WAITING FOR %s TO ACCEPT").c_str(), entry.username.c_str()), _("CANCEL"), nullptr),
	mEntry(entry),
	mPollDelay(STATE_POLL_MS),
	mClosing(false)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	writer.StartObject();
	writer.Key("username");
	writer.String(SystemConf::getInstance()->get("global.netplay.nickname").c_str());
	writer.EndObject();

	HttpReqOptions options;
	options.dataToPost = buffer.GetString();
	options.customHeaders.push_back("Content-Type: application/json");
	mRequest = std::unique_ptr<HttpReq>(new HttpReq(challengeUrl(), &options));
}

GuiNetPlayJoin::~GuiNetPlayJoin()
{
	if (mToken.empty() || mClosing)
		return;

	sendChallengeRequest("DELETE");
	Uint32 deadline = SDL_GetTicks() + CANCEL_TIMEOUT_MS;
	while (mRequest->status() == HttpReq::REQ_IN_PROGRESS && SDL_GetTicks() < deadline)
		SDL_Delay(10);
}

std::string GuiNetPlayJoin::challengeUrl() const
{
	return HipposLobby::url("/rooms/" + mEntry.room_id + "/challenge");
}

void GuiNetPlayJoin::sendChallengeRequest(const std::string& method)
{
	HttpReqOptions options;
	options.method = method;
	options.customHeaders.push_back("Authorization: Bearer " + mToken);
	mRequest = std::unique_ptr<HttpReq>(new HttpReq(challengeUrl(), &options));
}

void GuiNetPlayJoin::update(int deltaTime)
{
	GuiMsgBox::update(deltaTime);

	if (mRequest)
	{
		auto status = mRequest->status();
		if (status == HttpReq::REQ_IN_PROGRESS)
			return;

		std::string content = status == HttpReq::REQ_SUCCESS ? mRequest->getContent() : "";
		mRequest.reset();

		if (status == HttpReq::REQ_404_NOTFOUND || status == 409)
			closeWithMessage(mToken.empty() ? _("SOMEONE ELSE ALREADY JOINED THIS GAME") : _("THIS GAME IS NO LONGER AVAILABLE"));
		else if (status != HttpReq::REQ_SUCCESS)
			closeWithMessage(_("THE NETPLAY LOBBY CAN'T BE REACHED"));
		else if (mToken.empty())
			readChallenge(content);
		else
			readChallengeState(content);
		return;
	}

	mPollDelay -= deltaTime;
	if (mPollDelay <= 0)
	{
		mPollDelay = STATE_POLL_MS;
		sendChallengeRequest("GET");
	}
}

void GuiNetPlayJoin::readChallenge(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("token") || !doc["token"].IsString())
	{
		closeWithMessage(_("THE NETPLAY LOBBY CAN'T BE REACHED"));
		return;
	}

	mToken = doc["token"].GetString();
}

void GuiNetPlayJoin::readChallengeState(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("state") || !doc["state"].IsString())
		return;

	std::string state = doc["state"].GetString();
	if (state == "declined")
	{
		closeWithMessage(Utils::String::format(_("%s DECLINED YOUR CHALLENGE").c_str(), mEntry.username.c_str()));
		return;
	}

	if (state != "accepted" || !doc.HasMember("ip") || !doc["ip"].IsString())
		return;

	LaunchGameOptions options;
	options.netPlayMode = CLIENT;
	options.ip = doc["ip"].GetString();
	options.port = doc.HasMember("port") && doc["port"].IsInt() ? doc["port"].GetInt() : mEntry.port;
	options.core = mEntry.core_name;
	options.emulator = mEntry.emulator;

	mClosing = true;
	launchFromNetplayLobby(mWindow, mEntry.fileData, options);
}

void GuiNetPlayJoin::closeWithMessage(const std::string& message)
{
	Window* window = mWindow;
	mClosing = true;
	delete this;
	window->pushGui(new GuiMsgBox(window, message));
}
