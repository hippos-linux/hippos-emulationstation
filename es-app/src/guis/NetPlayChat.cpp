#include "guis/NetPlayChat.h"

#include "HipposLobby.h"
#include "HttpReq.h"
#include "LocaleES.h"
#include "Settings.h"
#include "SystemConf.h"
#include "ThemeData.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

const std::string NetPlayChat::GLOBAL_CHANNEL = "global";

static const int POLL_INTERVAL_MS = 3000;

static std::vector<std::string> mutedPlayers()
{
	return Utils::String::commaStringToVector(SystemConf::getInstance()->get("global.netplay.muted"));
}

static bool isMuted(const std::string& username)
{
	auto muted = mutedPlayers();
	return std::find(muted.cbegin(), muted.cend(), username) != muted.cend();
}

NetPlayChat::NetPlayChat(Window* window)
	: ComponentGrid(window, Vector2i(1, 2)),
	mChannel(GLOBAL_CHANNEL),
	mLastMessageId(0),
	mPollDelay(0)
{
	auto theme = ThemeData::getMenuTheme();

	mChannelLabel = std::make_shared<TextComponent>(mWindow, "", theme->Text.font, theme->Text.color, ALIGN_CENTER);
	setEntry(mChannelLabel, Vector2i(0, 0), false, true);

	mMessages = std::make_shared<ComponentList>(mWindow);
	setEntry(mMessages, Vector2i(0, 1), true, true);

	setRowHeightPerc(0, 0.08f, false);
	setChannel(GLOBAL_CHANNEL, _("GLOBAL CHAT"));
}

NetPlayChat::~NetPlayChat() = default;

std::string NetPlayChat::chatUrl() const
{
	return HipposLobby::url("/chat/" + mChannel);
}

void NetPlayChat::setChannel(const std::string& channel, const std::string& label)
{
	mChannel = channel;
	mChannelLabel->setText(label);
	mMessages->clear();
	mLastMessageId = 0;
	mPollRequest.reset();
	mPollDelay = 0;
}

void NetPlayChat::startPoll()
{
	if (chatUrl().empty())
		return;

	mPollRequest = std::unique_ptr<HttpReq>(new HttpReq(chatUrl() + "?after=" + std::to_string(mLastMessageId)));
}

void NetPlayChat::update(int deltaTime)
{
	ComponentGrid::update(deltaTime);

	if (mSendRequest && mSendRequest->status() != HttpReq::REQ_IN_PROGRESS)
		finishSend();

	if (mPollRequest)
	{
		auto status = mPollRequest->status();
		if (status == HttpReq::REQ_IN_PROGRESS)
			return;

		if (status == HttpReq::REQ_SUCCESS)
			readMessages(mPollRequest->getContent());

		mPollRequest.reset();
		mPollDelay = POLL_INTERVAL_MS;
		return;
	}

	mPollDelay -= deltaTime;
	if (mPollDelay <= 0)
		startPoll();
}

void NetPlayChat::readMessages(const std::string& json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("messages") || !doc["messages"].IsArray())
		return;

	bool atNewest = mMessages->size() == 0 || mMessages->getCursorId() == mMessages->size() - 1;

	for (auto& message : doc["messages"].GetArray())
	{
		if (!message.HasMember("id") || !message["id"].IsInt() || !message.HasMember("username") || !message["username"].IsString() ||
			!message.HasMember("text") || !message["text"].IsString())
			continue;

		mLastMessageId = std::max(mLastMessageId, message["id"].GetInt());

		std::string username = message["username"].GetString();
		if (!isMuted(username))
			addMessage(username, message["text"].GetString());
	}

	if (atNewest && mMessages->size() > 0)
		mMessages->setCursorIndex(mMessages->size() - 1);
}

void NetPlayChat::addMessage(const std::string& username, const std::string& text)
{
	auto theme = ThemeData::getMenuTheme();

	ComponentListRow row;
	auto line = std::make_shared<TextComponent>(mWindow, username + ": " + text, theme->TextSmall.font, theme->Text.color);
	row.addElement(line, true);
	row.makeAcceptInputHandler([this, username] { confirmMute(username); });
	mMessages->addRow(row);
}

void NetPlayChat::say()
{
	auto send = [this](const std::string& text) { sendMessage(Utils::String::trim(text)); };

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("SAY"), "", send, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, _("SAY"), "", send, false));
}

void NetPlayChat::sendMessage(const std::string& text)
{
	if (text.empty() || mSendRequest)
		return;

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	writer.StartObject();
	writer.Key("username");
	writer.String(SystemConf::getInstance()->get("global.netplay.nickname").c_str());
	writer.Key("text");
	writer.String(text.c_str());
	writer.EndObject();

	HttpReqOptions options;
	options.dataToPost = buffer.GetString();
	options.customHeaders.push_back("Content-Type: application/json");
	mSendRequest = std::unique_ptr<HttpReq>(new HttpReq(chatUrl(), &options));
}

void NetPlayChat::finishSend()
{
	auto status = mSendRequest->status();
	mSendRequest.reset();

	if (status == HttpReq::REQ_429_TOOMANYREQUESTS)
		mWindow->displayNotificationMessage(_("YOU ARE SENDING MESSAGES TOO FAST"));
	else if (status == HttpReq::REQ_403_BADLOGIN)
		mWindow->displayNotificationMessage(_("YOU CAN'T SEND MESSAGES TO THIS LOBBY"));
	else if (status != HttpReq::REQ_SUCCESS)
		mWindow->displayNotificationMessage(_("MESSAGE NOT SENT"));

	mPollDelay = 0;
}

void NetPlayChat::confirmMute(const std::string& username)
{
	if (username == SystemConf::getInstance()->get("global.netplay.nickname"))
		return;

	mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::format(_("HIDE MESSAGES FROM %s?").c_str(), username.c_str()),
		_("YES"), [this, username]
		{
			auto muted = mutedPlayers();
			muted.push_back(username);
			SystemConf::getInstance()->set("global.netplay.muted", Utils::String::vectorToCommaString(muted));
			SystemConf::getInstance()->saveSystemConf();
			setChannel(mChannel, mChannelLabel->getValue());
		},
		_("NO"), nullptr));
}

std::vector<HelpPrompt> NetPlayChat::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_OK, _("HIDE PLAYER")));
	return prompts;
}
