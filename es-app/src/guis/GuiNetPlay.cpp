#include "GuiNetPlay.h"
#include "Window.h"
#include <string>
#include <future>
#include <fcntl.h>
#include "Log.h"
#include "Settings.h"
#include "SystemConf.h"
#include "LocaleES.h"
#include "HttpReq.h"
#include "AsyncHandle.h"
#include "guis/GuiMsgBox.h"
#include "SystemData.h"
#include "FileData.h"
#include "ThemeData.h"
#include "components/MenuComponent.h"
#include "components/ButtonComponent.h"
#include "views/ViewController.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiNetPlayHost.h"
#include "guis/NetPlayChat.h"
#include "ApiSystem.h"
#include "DolphinGameId.h"
#include "Paths.h"
#include "utils/FileSystemUtil.h"

#include <map>

#include <rapidjson/rapidjson.h>
#include <rapidjson/pointer.h>

#include "animations/LambdaAnimation.h"

#if WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")  // Link with Winsock library
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

static const std::string DOLPHIN_LOBBY_URL = "https://lobby.dolphin-emu.org";
static const std::string DOLPHIN_EMULATOR = "dolphin-emu";

// http://lobby.libretro.com/list/
// Core list :
// FCEUmm, FB Alpha, MAME 2000, MAME 2003, MAME 2010

static std::map<std::string, std::string> coreList =
{
#if WIN32
	{ "2048", "2048" },
	{ "81", "81" },
	{ "Atari800", "atari800" },
	{ "blueMSX", "bluemsx" },
	{ "Cannonball", "cannonball" },
	{ "Caprice32", "cap32" },
	{ "CrocoDS", "crocods" },
	{ "Daphne", "daphne" },
	{ "Dinothawr", "dinothawr" },
	{ "DOSBox-SVN", "dosbox_svn" },
	{ "EasyRPG Player", "easyrpg" },
	{ "FB Alpha 2012 Neo Geo", "fbalpha2012_neogeo" },
	{ "FB Alpha 2012", "fbalpha2012" },
	{ "FB Alpha", "fbalpha" },
	{ "FinalBurn Neo", "fbneo" },
	{ "FCEUmm", "fceumm" },
	{ "Flycast", "flycast" },
	{ "FreeIntv", "freeintv" },
	{ "Fuse", "fuse" },
	{ "Gambatte", "gambatte" },
	{ "Gearboy", "gearboy" },
	{ "Gearsystem", "gearsystem" },
	{ "Genesis Plus GX", "genesis_plus_gx" },
	{ "Game Music Emu", "gme" },
	{ "gpSP", "gpsp" },
	{ "GW", "gw" },
	{ "Handy", "handy" },
	{ "Hatari", "hatari" },
	{ "MAME 2003 (0.78)", "mame2003" },
	{ "MAME 2003-Plus", "mame2003_plus" },
	{ "MAME 2010 (0.139)", "mame2010" },
	{ "MAME 2016 (0.174)", "mame2016" },
	{ "Beetle Lynx", "mednafen_lynx" },
	{ "Beetle NeoPop", "mednafen_ngp" },
	{ "Beetle PCE Fast", "mednafen_pce_fast" },
	{ "Beetle PC-FX", "mednafen_pcfx" },
	{ "Beetle SuperGrafx", "mednafen_supergrafx" },
	{ "Beetle VB", "mednafen_vb" },
	{ "Beetle PSX", "mednafen_psx" },
	{ "Beetle PSX HW", "mednafen_psx_hw" },	
	{ "Beetle WonderSwan", "mednafen_wswan" },
	{ "Mesen-S", "mesen-s" },
	{ "mGBA", "mgba" },
	{ "Mr.Boom", "mrboom" },
	{ "Mupen64Plus-Next", "mupen64plus_next" },
	{ "Neko Project II", "nekop2" },
	{ "NeoCD", "neocd" },
	{ "NestopiaCV", "nestopiaCV" },
	{ "Nestopia", "nestopia" },
	{ "Neko Project II", "np2kai" },
	{ "NXEngine", "nxengine" },
	{ "O2EM", "o2em" },
	{ "Opera", "opera" },
	{ "ParaLLEl N64", "parallel_n64" },
	{ "PCSX-ReARMed", "pcsx_rearmed" },
	{ "PicoDrive", "picodrive" },
	{ "PokeMini", "pokemini" },
	{ "PPSSPP", "ppsspp" },
	{ "PrBoom", "prboom" },
	{ "ProSystem", "prosystem" },
	{ "PUAE", "puae" },
	{ "PX68k", "px68k" },
	{ "QUASI88", "quasi88" },
	{ "QuickNES", "quicknes" },
	{ "REminiscence", "reminiscence" },
	{ "SameBoy", "sameboy" },
	{ "ScummVM", "scummvm" },
	{ "Snes9x 2002", "snes9x2002" },
	{ "Snes9x 2005 Plus", "snes9x2005_plus" },
	{ "Snes9x 2010", "snes9x2010" },
	{ "Snes9x", "snes9x" },
	{ "Stella", "stella" },
	{ "TGB Dual", "tgbdual" },
	{ "TyrQuake", "tyrquake" },
	{ "UAE4ARM", "uae4arm" },
	{ "uzem", "uzem" },
	{ "VBA-M", "vbam" },
	{ "VBA Next", "vba_next" },
	{ "vecx", "vecx" },
	{ "VICE", "vice_x128" },
	{ "VICE", "vice_x64" },
	{ "VICE", "vice_xplus4" },
	{ "VICE", "vice_xvic" },
	{ "Virtual Jaguar", "virtualjaguar" },
	{ "x1", "x1" },
	{ "XRick", "xrick" },
	{ "YabaSanshiro", "yabasanshiro" },
	{ "Yabause", "yabause" },
#else
	{ "Beetle NeoPop", "mednafen_ngp" },
	{ "Beetle PCE Fast", "pce" },
	{ "Beetle SuperGrafx", "mednafen_supergrafx" },
	{ "Beetle VB", "vb" },
	{ "Beetle WonderSwan", "mednafen_wswan" },
	{ "DOSBox-SVN", "dosbox" },
	{ "EightyOne", "81" },
	{ "FB Alpha 2012 Neo Geo", "fbalpha2012_neogeo" },
	{ "FB Alpha 2012", "fbalpha2012" },
	{ "FB Alpha", "fbalpha" },
	{ "FinalBurn Neo", "fbneo" },
	{ "Game & Watch", "gw" },
	{ "Genesis Plus GX", "genesisplusgx" },
	{ "MAME 2000", "imame4all" },
	{ "MAME 2003 (0.78)", "mame2003" },
	{ "MAME 2003-Plus", "mame078plus" },
	{ "MAME 2010", "mame0139" },
	{ "MAME 2014", "mame2014" },
	{ "PCSX-ReARMed", "pcsx_rearmed" },
	{ "Snes9x 2002", "pocketsnes" },
	{ "Snes9x 2010", "snes9x_next" },
	{ "TGB Dual", "tgbdual", },
	{ "VICE x64", "vice" }
#endif
};

void GuiNetPlay::open(Window* window)
{
	if (ApiSystem::getInstance()->getIpAddress() == "NOT CONNECTED")
		window->pushGui(new GuiMsgBox(window, _("YOU ARE NOT CONNECTED TO A NETWORK"), _("OK"), nullptr));
	else
		window->pushGui(new GuiNetPlay(window));
}

GuiNetPlay::GuiNetPlay(Window* window)
	: GuiComponent(window), 
	mBusyAnim(window),
	mBackground(window, ":/frame.png"),
	mGrid(window, Vector2i(2, 3)),
	mList(nullptr),
	mLanLobbySocket(-1),
	mLanLobbySocketTimeout(0),
	mPopulateThread(nullptr),
	mThreadFinished(false)
{	
	addChild(&mBackground);
	addChild(&mGrid);

	// Form background
	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);
	mBackground.setPostProcessShader(theme->Background.menuShader);

	// Title

	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 5));

	mTitle = std::make_shared<TextComponent>(mWindow, _("NETPLAY LOBBY"), theme->Title.font, theme->Title.color, ALIGN_CENTER);
	mSubtitle = std::make_shared<TextComponent>(mWindow, _("Join a game, host your own or chat"), theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);
	
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true, Vector2i(2, 1));

	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 1), true, true, Vector2i(1, 1), GridFlags::BORDER_TOP | GridFlags::BORDER_RIGHT);

	mChat = std::make_shared<NetPlayChat>(mWindow);
	mGrid.setEntry(mChat, Vector2i(1, 1), true, true, Vector2i(1, 1), GridFlags::BORDER_TOP);

	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("HOST A GAME"), _("HOST A GAME"), [this] { openNetplayHostPicker(mWindow); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("SAY"), _("SAY"), [this] { mChat->say(); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CHAT ROOM"), _("CHAT ROOM"), [this] { chooseChatRoom(); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("REFRESH"), _("REFRESH"), [this] { startRequest(); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CLOSE"), _("CLOSE"), [this] { delete this; }));

	mButtonGrid = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false, Vector2i(2, 1), GridFlags::BORDER_TOP);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool 
	{
		if (config->isMappedLike("down", input)) {
			mGrid.setCursorTo(mList);
			mList->setCursorIndex(0);
			return true;
		}
		if (config->isMappedLike("up", input)) {
			mList->setCursorIndex(mList->size() - 1);
			mGrid.moveCursor(Vector2i(0, 1));
			return true;
		}
		return false;
	});

	if (Renderer::ScreenSettings::fullScreenMenus())
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		setSize(Renderer::getScreenWidth() * 0.95f, Renderer::getScreenHeight() * 0.90f);

	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);

	// Loading
    mBusyAnim.setSize(Vector2f(Renderer::getScreenWidth(), Renderer::getScreenHeight()));
	mBusyAnim.setText(_("PLEASE WAIT"));
	startRequest();
}

GuiNetPlay::~GuiNetPlay()
{
	if (mLanLobbySocket >= 0)
	{

#if WIN32
		closesocket(mLanLobbySocket);
		WSACleanup();
#else
		close(mLanLobbySocket);
#endif

		mLanLobbySocket = -1;
	}
}

void GuiNetPlay::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	
	mGrid.setSize(mSize);
	
	const float titleHeight = mTitle->getFont()->getLetterHeight();
	const float subtitleHeight = mSubtitle->getFont()->getLetterHeight();
	const float titleSubtitleSpacing = mSize.y() * 0.03f;

	mGrid.setRowHeight(0, (titleHeight + titleSubtitleSpacing + subtitleHeight + TITLE_VERT_PADDING));
	mGrid.setRowHeight(2, mButtonGrid->getSize().y());
	mGrid.setColWidthPerc(0, 0.6f);

	mHeaderGrid->setRowHeight(1, titleHeight);
	mHeaderGrid->setRowHeight(2, titleSubtitleSpacing);
	mHeaderGrid->setRowHeight(3, subtitleHeight);
}

void GuiNetPlay::startRequest()
{
	if (!mLobbySources.empty())
		return;

	mList->clear();
	mLobbyEntries.clear();
	mLanEntries.clear();

	lanLobbyRequest();

	std::string netPlayLobby = SystemConf::getInstance()->get("global.netplay.lobby");
	if (netPlayLobby.empty())
		netPlayLobby = "http://lobby.libretro.com/list/";

	LobbySource libretroLobby;
	libretroLobby.request = std::unique_ptr<HttpReq>(new HttpReq(netPlayLobby));
	mLobbySources.push_back(std::move(libretroLobby));

	std::string hipposLobbyUrl = SystemConf::getInstance()->get("global.netplay.hipposlobby");
	if (!hipposLobbyUrl.empty())
	{
		if (hipposLobbyUrl.back() != '/')
			hipposLobbyUrl += "/";

		LobbySource hipposLobby;
		hipposLobby.request = std::unique_ptr<HttpReq>(new HttpReq(hipposLobbyUrl + "list"));
		hipposLobby.kind = LobbyKind::HIPPOS;
		mLobbySources.push_back(std::move(hipposLobby));
	}

	std::string dolphinVersionFile = Paths::getRootPath() + "/emulators/" + DOLPHIN_EMULATOR + "/version";
	std::string dolphinVersion = Utils::FileSystem::exists(dolphinVersionFile) ? Utils::String::trim(Utils::FileSystem::readAllText(dolphinVersionFile)) : "";
	if (!dolphinVersion.empty())
	{
		LobbySource dolphinLobby;
		dolphinLobby.request = std::unique_ptr<HttpReq>(new HttpReq(DOLPHIN_LOBBY_URL + "/v0/list?version=" + HttpReq::urlEncode(dolphinVersion) + "&password=0&in_game=0"));
		dolphinLobby.kind = LobbyKind::DOLPHIN;
		mLobbySources.push_back(std::move(dolphinLobby));
	}
}

void GuiNetPlay::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mLanLobbySocketTimeout < 20000 && mPopulateThread == nullptr) // allow receiving answers from the LAN for 20 seconds
	{
		mLanLobbySocketTimeout += deltaTime;
		populateFromLan();
	}

	if (mPopulateThread != nullptr)
	{
		mBusyAnim.update(deltaTime);

		if (mThreadFinished)
		{
			populateList();

			if (mList->size() != 0)
				mList->setCursorIndex(0, true);

			mLobbySources.clear();

			mPopulateThread->join();
			delete mPopulateThread;
			mPopulateThread = nullptr;
		}
		return;
	}

	if (mLobbySources.empty())
		return;

	bool pending = false;
	for (auto& source : mLobbySources)
	{
		if (!source.request)
			continue;

		auto status = source.request->status();
		if (status == HttpReq::REQ_IN_PROGRESS)
		{
			pending = true;
			continue;
		}

		if (status == HttpReq::REQ_SUCCESS)
			source.content = source.request->getContent();
		else if (source.kind != LobbyKind::LIBRETRO)
			LOG(LogWarning) << "GuiNetPlay - lobby request failed: " << source.request->getUrl() << " " << source.request->getErrorMsg();
		else
			mWindow->pushGui(new GuiMsgBox(mWindow, _("FAILED") + std::string(" : ") + source.request->getErrorMsg()));

		source.request.reset();
	}

	if (pending)
	{
		mBusyAnim.update(deltaTime);
		return;
	}

	std::vector<std::pair<std::string, LobbyKind>> responses;
	for (auto& source : mLobbySources)
		if (!source.content.empty())
			responses.push_back({ source.content, source.kind });

	if (responses.empty())
	{
		mLobbySources.clear();
		return;
	}

	mThreadFinished = false;
	mPopulateThread = new std::thread([this, responses]() { populateFromLobbies(responses); });
}

#if WIN32
static std::vector<std::string> getBroadcastAddresses() 
{
	std::vector<std::string> ret;

	PIP_ADAPTER_ADDRESSES pAddresses = nullptr, pCurrAddress = nullptr;
	ULONG outBufLen = 15000;
	DWORD dwRetVal;

	pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
	if (pAddresses == nullptr)
		return ret;	

	dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
	if (dwRetVal != NO_ERROR) 
	{
		free(pAddresses);
		return ret;
	}
	
	for (pCurrAddress = pAddresses; pCurrAddress != nullptr; pCurrAddress = pCurrAddress->Next) 
	{
		if (pCurrAddress->OperStatus == IfOperStatusUp && pCurrAddress->FirstUnicastAddress) 
		{
			sockaddr_in* sa = (sockaddr_in*)pCurrAddress->FirstUnicastAddress->Address.lpSockaddr;
			sockaddr_in* mask = (sockaddr_in*)pCurrAddress->FirstUnicastAddress->OnLinkPrefixLength;

			if (sa->sin_family != AF_INET)
				continue;
			
			uint32_t ip = sa->sin_addr.s_addr;
			uint32_t subnetMask = htonl(~((1 << (32 - pCurrAddress->FirstUnicastAddress->OnLinkPrefixLength)) - 1));
			uint32_t broadcast = ip | ~subnetMask;

			struct in_addr broadcastInAddr;
			broadcastInAddr.s_addr = broadcast;
				
			std::string broadcastAddr = inet_ntoa(broadcastInAddr);
			ret.push_back(broadcastAddr);							
		}
	}

	if (ret.size() > 1)
	{
		// Probably : VirtualBox adapter
		auto it = std::find(ret.cbegin(), ret.cend(), "192.168.56.255");
		if (it != ret.cend())
			ret.erase(it);
	}
	
	free(pAddresses);
	return ret;
}
#endif

void GuiNetPlay::lanLobbyRequest()
{
	int port = Utils::String::toInteger(SystemConf::getInstance()->get("global.netplay.port"));

	mLanLobbySocketTimeout = 0;

	if (mLanLobbySocket < 0)
	{
#if WIN32
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			return;

		mLanLobbySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (mLanLobbySocket < 0)
			return;

		int broadcastEnable = 1;
		setsockopt(mLanLobbySocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastEnable, sizeof(broadcastEnable));
	
		int on = 1;
		setsockopt(mLanLobbySocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		struct sockaddr_in bindAddr;
		memset(&bindAddr, 0, sizeof(bindAddr));
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(0);
		bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(mLanLobbySocket, (struct sockaddr*)&bindAddr, sizeof(bindAddr)) < 0) 
		{
			closesocket(mLanLobbySocket);
			WSACleanup();
			return;
		}

		u_long mode = 1;
		if (ioctlsocket(mLanLobbySocket, FIONBIO, &mode) != 0) 
		{		
			closesocket(mLanLobbySocket);
			WSACleanup();
			return;
		}
#else 
		mLanLobbySocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (mLanLobbySocket < 0)
			return;

		int broadcastEnable = 1;
		setsockopt(mLanLobbySocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

		int flags = fcntl(mLanLobbySocket, F_GETFL, 0);
		fcntl(mLanLobbySocket, F_SETFL, flags | O_NONBLOCK);
#endif
	}

	uint32_t query_magic = htonl(DISCOVERY_QUERY_MAGIC);

#if WIN32
	for (auto address : getBroadcastAddresses())
	{
		struct sockaddr_in broadcastAddr;
		memset(&broadcastAddr, 0, sizeof(broadcastAddr));
		broadcastAddr.sin_family = AF_INET;
		broadcastAddr.sin_port = htons(port);		
		broadcastAddr.sin_addr.s_addr = inet_addr(address.c_str());		

		sendto(mLanLobbySocket, (const char*)&query_magic, sizeof(query_magic), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
	}
#else
	struct sockaddr_in broadcastAddr;
	memset(&broadcastAddr, 0, sizeof(broadcastAddr));
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_port = htons(port);
	broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	sendto(mLanLobbySocket, &query_magic, sizeof(query_magic), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
#endif	
}

bool GuiNetPlay::input(InputConfig* config, Input input)
{
	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		delete this;
		return true;
	}

	if (input.value != 0 && config->isMappedTo("y", input))
	{
		mChat->say();
		return true;
	}

	if (input.value != 0 && config->isMappedTo("x", input))
	{
		openNetplayHostPicker(mWindow);
		return true;
	}

	return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiNetPlay::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("x", _("HOST A GAME")));
	prompts.push_back(HelpPrompt("y", _("SAY")));
	return prompts;
}


FileData* GuiNetPlay::getFileData(const std::string& gameInfo, bool crc, const std::string& coreName)
{
	auto normalizeName = [](const std::string& name)
	{
		auto ret = Utils::String::toLower(name);
		ret = Utils::String::replace(ret, "_", " ");
		ret = Utils::String::replace(ret, ".", "");
		ret = Utils::String::replace(ret, "'", "");
		return Utils::String::removeParenthesis(ret);
	};

	std::string lowCore;

	auto coreInfo = coreList.find(coreName);
	if (coreInfo != coreList.cend())
		lowCore = Utils::String::toLower(coreInfo->second);
	else
		lowCore = Utils::String::toLower(Utils::String::replace(coreName, " ", "_"));
	
	std::string normalizedName = normalizeName(gameInfo);
	std::string normalizedNameNoSpace = Utils::String::replace(normalizedName, " ", "");

	std::vector<std::future<FileData*>> futures;
	for (auto sys : SystemData::sSystemVector)
	{
		if (!sys->isNetplaySupported())
			continue;

		if (!crc)
		{
			bool coreExists = false;

			for (auto& emul : sys->getEmulators())
				for (auto& core : emul.cores)
					if (Utils::String::toLower(core.name) == lowCore)
					{
						coreExists = true;
						break;
					}

			if (!coreExists)
				continue;
		}

		futures.push_back(std::async(std::launch::async, [sys, crc, gameInfo, normalizedName, normalizedNameNoSpace, &normalizeName]() -> FileData* {
			for (auto file : sys->getRootFolder()->getFilesRecursive(GAME, false, sys))
			{
				if (crc)
				{
					if (file->getMetadata(MetaDataId::Crc32) == gameInfo)
						return file;

					continue;
				}
				else
				{
					std::string stem = normalizeName(file->getName());
					if (stem == normalizedName)
						return file;

					stem = normalizeName(Utils::FileSystem::getStem(file->getPath()));
					if (stem == normalizedName)
						return file;

					stem = Utils::String::replace(normalizeName(file->getName()), " ", "");
					if (stem == Utils::String::replace(normalizedName, " ", ""))
						return file;
				}
			}
			return nullptr;
		}));
	}

	for (auto& fut : futures)
	{
		FileData* file = fut.get();
		if (file != nullptr)
			return file;
	}

	return nullptr;
}

class NetPlayLobbyListEntry : public ComponentGrid
{
public:
	NetPlayLobbyListEntry(Window* window, LobbyAppEntry& entry) :
		ComponentGrid(window, Vector2i(4, 3))
	{
		mEntry = entry;

		auto theme = ThemeData::getMenuTheme();

		mImage = std::make_shared<ImageComponent>(mWindow);
		// mImage->setIsLinear(true);

		std::string name = entry.fileData == nullptr ? entry.game_name : entry.fileData->getMetadata(MetaDataId::Name) + " [" + entry.fileData->getSystemName() + "]";

		mText = std::make_shared<TextComponent>(mWindow, name.c_str(), theme->Text.font, theme->Text.color);
		mText->setLineSpacing(1.5);
		mText->setVerticalAlignment(ALIGN_TOP);

		std::string userInfo = _U("\uf007  ") + entry.username + _U("  \uf0AC  ") + entry.country + _U("  \uf108  ") + entry.frontend; // _U("  \uf0E8  ") + entry.ip +  

		mSubstring = std::make_shared<TextComponent>(mWindow, userInfo.c_str(), theme->TextSmall.font, theme->Text.color);
		mSubstring->setOpacity(192);

		std::string subInfo = _U("\uf11B  ") + entry.core_name + " (" + entry.retroarch_version + ")";

		if (entry.fileData != nullptr)
		{
			if (!entry.isCrcValid)
			{
				if (entry.game_crc == "00000000")
					subInfo = subInfo + "   " + _U("\uf059  ") + _("UNKNOWN ROM VERSION");
				else
					subInfo = subInfo + "   " + _U("\uf05E  ") + _("DIFFERENT ROM");
			}
			else
				subInfo = subInfo + "  " + _U("\uf058  ") + _("SAME ROM");
		}

		if (entry.fileData != nullptr && !entry.coreExists)
			subInfo = subInfo + "   " + _U("\uf071  ") + _("UNAVAILABLE CORE");
		
		mDetails = std::make_shared<TextComponent>(mWindow, subInfo.c_str(), theme->TextSmall.font, theme->Text.color);
		mDetails->setOpacity(192);
		
		if (entry.has_password || entry.has_spectate_password)
		{					
			if (!entry.has_spectate_password)
				mLockInfo = std::make_shared<TextComponent>(mWindow, std::string(_U("\uf06E")).c_str(), Font::get(FONT_SIZE_MEDIUM, FONT_PATH_REGULAR), theme->Text.color);
			else
				mLockInfo = std::make_shared<TextComponent>(mWindow, std::string(_U("\uf023")).c_str(), Font::get(FONT_SIZE_MEDIUM, FONT_PATH_REGULAR), theme->Text.color);

			mLockInfo->setOpacity(192);
			mLockInfo->setLineSpacing(1);
		}

		setEntry(mImage, Vector2i(0, 0), false, true, Vector2i(1, 3));
		setEntry(mText, Vector2i(2, 0), false, true);
		setEntry(mSubstring, Vector2i(2, 1), false, true);
		setEntry(mDetails, Vector2i(2, 2), false, true);
		
		if (mLockInfo != nullptr)
			setEntry(mLockInfo, Vector2i(3, 0), false, true, Vector2i(1, 3));		
		
		float rowHeight = mText->getSize().y() * 1.1f + mSubstring->getSize().y() + mDetails->getSize().y();
		float imageColWidth = rowHeight * 1.15f;

		float sw = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));
		
		mImage->setOrigin(0.5f, 0.5f);
		mImage->setMaxSize(imageColWidth, rowHeight);
		mImage->setPadding(Vector4f(4.0f));

		if (entry.fileData == nullptr)
		{
			mImage->setColorShift(0x80808080);
			mImage->setOpacity(120);
			mText->setOpacity(150);
			mSubstring->setOpacity(120);
			mDetails->setOpacity(120);
		}

		setColWidth(0, imageColWidth, false);
		setColWidthPerc(1, 0.015f, false);
		setColWidthPerc(3, mLockInfo != nullptr ? 0.055f : 0.002f, false); // cf FONT_SIZE_LARGE

		setRowHeight(0, mText->getSize().y(), false);
		setRowHeight(1, mSubstring->getSize().y(), false);
		setRowHeight(2, mDetails->getSize().y(), false);

		setSize(Vector2f(0, rowHeight));

		if (entry.fileData == nullptr || !Utils::FileSystem::exists(entry.fileData->getImagePath()))
			mImage->setImage(":/cartridge.svg");
		else
		{
			mImage->setRoundCorners(0.08f);
			mImage->setImage(entry.fileData->getImagePath());
		}
	}

	LobbyAppEntry& getEntry() {
		return mEntry;
	}

	virtual void setColor(unsigned int color)
	{
		mText->setColor(color);
		mSubstring->setColor(color);
		mDetails->setColor(color);

		if (mLockInfo != nullptr)
			mLockInfo->setColor(color);
	}

private:
	std::shared_ptr<ImageComponent>  mImage;

	std::shared_ptr<TextComponent>  mText;
	std::shared_ptr<TextComponent>  mSubstring;
	std::shared_ptr<TextComponent>  mDetails;
	std::shared_ptr<TextComponent>	mLockInfo;

	LobbyAppEntry mEntry;	
};

bool GuiNetPlay::populateList()
{
	std::vector<LobbyAppEntry> owned;
	std::vector<LobbyAppEntry> missing;

	for (auto& entries : { mLanEntries, mLobbyEntries })
		for (auto& game : entries)
			(game.fileData != nullptr && game.coreExists ? owned : missing).push_back(game);

	std::stable_sort(owned.begin(), owned.end(), [](const LobbyAppEntry& a, const LobbyAppEntry& b)
	{
		int bySystem = Utils::String::compareIgnoreCase(a.fileData->getSystem()->getFullName(), b.fileData->getSystem()->getFullName());
		if (bySystem != 0)
			return bySystem < 0;

		return Utils::String::compareIgnoreCase(a.fileData->getName(), b.fileData->getName()) < 0;
	});

	std::string group;
	for (auto& game : owned)
	{
		const std::string& systemName = game.fileData->getSystem()->getFullName();
		if (systemName != group)
		{
			mList->addGroup(systemName, true);
			group = systemName;
		}

		ComponentListRow row;
		row.addElement(std::make_shared<NetPlayLobbyListEntry>(mWindow, game), true);
		row.makeAcceptInputHandler([this, game] { launchGame(game); });
		mList->addRow(std::move(row));
	}

	if (Settings::NetPlayShowMissingGames() && !missing.empty())
	{
		mList->addGroup(_("UNAVAILABLE GAMES"), true);

		for (auto& game : missing)
		{
			ComponentListRow row;
			row.addElement(std::make_shared<NetPlayLobbyListEntry>(mWindow, game), true);
			mList->addRow(std::move(row));
		}
	}

	if (mList->size() == 0)
	{
		ComponentListRow row;
		auto empty = std::make_shared<TextComponent>(mWindow);
		empty->setText(_("NO GAMES FOUND"));
		row.addElement(empty, true);
		mList->addRow(std::move(row));
	}

	return true;
}

void GuiNetPlay::chooseChatRoom()
{
	auto picker = new GuiSettings(mWindow, _("CHAT ROOM"));
	picker->addEntry(_("GLOBAL CHAT"), false, [this, picker]
	{
		mChat->setChannel(NetPlayChat::GLOBAL_CHANNEL, _("GLOBAL CHAT"));
		picker->close();
	});

	for (auto system : SystemData::sSystemVector)
	{
		if (system->isCollection() || system->isGroupChildSystem() || !system->isNetplaySupported() || system->getGameCount() == 0)
			continue;

		picker->addEntry(system->getFullName(), false, [this, picker, system]
		{
			mChat->setChannel(system->getName(), system->getFullName());
			picker->close();
		});
	}

	mWindow->pushGui(picker);
}

void GuiNetPlay::populateFromLobbies(std::vector<std::pair<std::string, LobbyKind>> responses)
{
	std::vector<LobbyAppEntry> entries;
	for (auto& response : responses)
	{
		if (response.second == LobbyKind::DOLPHIN)
			populateFromDolphinJson(response.first, entries);
		else
			populateFromJson(response.first, response.second == LobbyKind::HIPPOS, entries);
	}

	std::sort(entries.begin(), entries.end(), [](const LobbyAppEntry& a, const LobbyAppEntry& b) {
		return a.isCrcValid ? !b.isCrcValid : (a.coreExists && !b.coreExists);
		});

	mLobbyEntries = entries;
	mThreadFinished = true;
}

static std::map<std::string, FileData*> dolphinGamesById()
{
	std::map<std::string, FileData*> games;
	for (auto sys : SystemData::sSystemVector)
	{
		if (sys->isCollection())
			continue;

		auto emulators = sys->getEmulators();
		if (std::none_of(emulators.cbegin(), emulators.cend(), [](const EmulatorData& emul) { return emul.name == DOLPHIN_EMULATOR; }))
			continue;

		for (auto file : sys->getRootFolder()->getFilesRecursive(GAME, false, sys))
		{
			std::string id = readDolphinGameId(file->getPath());
			if (!id.empty())
				games.emplace(id, file);
		}
	}
	return games;
}

void GuiNetPlay::populateFromDolphinJson(const std::string& json, std::vector<LobbyAppEntry>& entries)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("sessions") || !doc["sessions"].IsArray())
	{
		LOG(LogError) << "GuiNetPlay - Error parsing Dolphin lobby JSON";
		return;
	}

	auto games = dolphinGamesById();

	for (auto& session : doc["sessions"].GetArray())
	{
		if (!session.HasMember("game") || !session["game"].IsString() || !session.HasMember("server_id") || !session["server_id"].IsString() ||
			!session.HasMember("method") || !session["method"].IsString() || !session.HasMember("port") || !session["port"].IsNumber())
			continue;

		LobbyAppEntry game;
		game.game_name = session["game"].GetString();
		game.ip = session["server_id"].GetString();
		game.port = session["port"].GetInt();
		if (std::string(session["method"].GetString()) == "traversal")
			game.traversal_code = game.ip;

		if (session.HasMember("name") && session["name"].IsString())
			game.username = session["name"].GetString();
		if (session.HasMember("region") && session["region"].IsString())
			game.country = session["region"].GetString();
		if (session.HasMember("version") && session["version"].IsString())
			game.retroarch_version = session["version"].GetString();

		game.frontend = "Dolphin";
		game.emulator = DOLPHIN_EMULATOR;
		game.core_name = DOLPHIN_EMULATOR;

		auto file = games.find(game.game_name);
		if (file != games.cend())
		{
			game.fileData = file->second;
			game.isCrcValid = true;
			game.coreExists = true;
		}

		entries.push_back(std::move(game));
	}
}

void GuiNetPlay::populateFromJson(const std::string& json, bool hipposLobby, std::vector<LobbyAppEntry>& entries)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (doc.HasParseError() || !doc.IsArray())
	{
		LOG(LogError) << "GuiNetPlay - Error parsing lobby JSON";
		return;
	}

	bool netPlayShowOnlyRelayServerGames = Settings::NetPlayShowOnlyRelayServerGames() && !hipposLobby;

	for (auto& item : doc.GetArray())
	{
		if (!item.HasMember("fields"))
			continue;

		const rapidjson::Value& fields = item["fields"];
		//if (fields.HasMember("has_password") && fields["has_password"].IsBool() && fields["has_password"].GetBool())
	//		continue;

		LobbyAppEntry game;	
		game.isCrcValid = false;

		if (fields.HasMember("host_method") && fields["host_method"].IsInt())
			game.host_method = fields["host_method"].GetInt();

		if (netPlayShowOnlyRelayServerGames && game.host_method != 3)
			continue;

		if (fields.HasMember("core_name") && fields["core_name"].IsString())
			game.core_name = fields["core_name"].GetString();

		if (hipposLobby && fields.HasMember("emulator") && fields["emulator"].IsString())
			game.emulator = fields["emulator"].GetString();

		FileData* file = nullptr;

		if (fields.HasMember("game_crc") && fields["game_crc"].IsString() && fields["game_crc"] != "00000000")
			file = getFileData(Utils::String::toUpper(fields["game_crc"].GetString()), true, game.core_name);

		if (file == nullptr && fields.HasMember("game_name") && fields["game_name"].IsString())
		{
			file = getFileData(fields["game_name"].GetString(), false, game.core_name);
			if (file != nullptr)
				file->checkCrc32();
		}

		game.fileData = file;

		if (fields.HasMember("username") && fields["username"].IsString())
			game.username = fields["username"].GetString();

		if (fields.HasMember("game_crc") && fields["game_crc"].IsString())
			game.game_crc = fields["game_crc"].GetString();

		if (file != nullptr)
		{
			std::string fileCRC = file->getMetadata(MetaDataId::Crc32);
			if (game.game_crc == fileCRC)
				game.isCrcValid = true;
		}

		if (fields.HasMember("mitm_ip") && fields["mitm_ip"].IsString())
			game.mitm_ip = fields["mitm_ip"].GetString();

		if (fields.HasMember("subsystem_name") && fields["subsystem_name"].IsString())
			game.subsystem_name = fields["subsystem_name"].GetString();

		if (fields.HasMember("frontend") && fields["frontend"].IsString())
			game.frontend = fields["frontend"].GetString();

		if (fields.HasMember("created") && fields["created"].IsString())
			game.created = fields["created"].GetString();

		if (fields.HasMember("ip") && fields["ip"].IsString())
			game.ip = fields["ip"].GetString();

		if (fields.HasMember("updated") && fields["updated"].IsString())
			game.updated = fields["updated"].GetString();

		if (fields.HasMember("country") && fields["country"].IsString())
			game.country = fields["country"].GetString();

		if (fields.HasMember("has_password") && fields["has_password"].IsBool())
			game.has_password = fields["has_password"].GetBool();

		if (fields.HasMember("game_name") && fields["game_name"].IsString())
			game.game_name = fields["game_name"].GetString();

		if (fields.HasMember("has_spectate_password") && fields["has_spectate_password"].IsBool())
			game.has_spectate_password = fields["has_spectate_password"].GetBool();		

		if (fields.HasMember("mitm_port") && fields["mitm_port"].IsInt())
			game.mitm_port = fields["mitm_port"].GetInt();

		if (fields.HasMember("mitm_session") && fields["mitm_session"].IsString())
			game.mitm_session = fields["mitm_session"].GetString();
		
		if (fields.HasMember("fixed") && fields["fixed"].IsBool())
			game.fixed = fields["fixed"].GetBool();

		if (fields.HasMember("retroarch_version") && fields["retroarch_version"].IsString())
			game.retroarch_version = fields["retroarch_version"].GetString();

		if (fields.HasMember("port") && fields["port"].IsInt())
			game.port = fields["port"].GetInt();

		game.coreExists = coreExists(file, game.core_name);

		entries.push_back(std::move(game));
	}
}

bool GuiNetPlay::populateFromLan()
{
	if (mLanLobbySocket < 0)
		return false;

	bool changed = false;

	struct sockaddr_in serverAddr;
	socklen_t addrLen = sizeof(serverAddr);
	ad_packet response;

	while (true)
	{
#if WIN32
		int received = recvfrom(mLanLobbySocket, (char*)&response, sizeof(response), 0, (struct sockaddr*)&serverAddr, &addrLen);
#else
		ssize_t received = recvfrom(mLanLobbySocket, &response, sizeof(response), 0, (struct sockaddr*)&serverAddr, &addrLen);
#endif
		if (received < 0)
			break;

		if (received != sizeof(response))
			continue;

		if (ntohl(response.header) != DISCOVERY_RESPONSE_MAGIC)
			continue;

		LobbyAppEntry game;

		game.isCrcValid = false;
		game.core_name = response.core;

		char crcBuf[9];
		snprintf(crcBuf, sizeof(crcBuf), "%08X", ntohl(response.content_crc));
		game.game_crc = std::string(crcBuf);

		FileData* file = getFileData(game.game_crc, true, game.core_name);
		if (file == nullptr)
		{
			file = getFileData(response.content, false, game.core_name);
			if (file != nullptr)
				file->checkCrc32();
		}

		game.fileData = file;
		game.username = response.nick;

		if (file != nullptr)
		{
			std::string fileCRC = file->getMetadata(MetaDataId::Crc32);
			if (game.game_crc == fileCRC)
				game.isCrcValid = true;
		}

		game.subsystem_name = response.subsystem_name;
		game.frontend = response.frontend;
		game.ip = inet_ntoa(serverAddr.sin_addr);
		game.country = "lan";
		game.has_password = ntohl(response.has_password);
		game.has_spectate_password = ntohl(response.has_password);
		game.game_name = response.content;
		game.retroarch_version = response.retroarch_version;
		game.core_version = response.core_version;
		game.port = ntohl(response.port);

		game.coreExists = coreExists(file, game.core_name);

		if (!std::any_of(mLanEntries.cbegin(), mLanEntries.cend(), [game](const LobbyAppEntry& entry) { return entry.country == "lan" && entry.ip == game.ip && entry.port == game.port && entry.username == game.username && entry.game_crc == game.game_crc; }))
			mLanEntries.push_back(std::move(game));
	}

	if (changed)
		populateList();

	return true;
}

void GuiNetPlay::render(const Transform4x4f &parentTrans) 
{
	GuiComponent::render(parentTrans);

	if (!mLobbySources.empty())
		mBusyAnim.render(parentTrans);
}

void GuiNetPlay::launchGame(LobbyAppEntry entry)
{
	LaunchGameOptions options;
	options.netPlayMode = CLIENT;

	if (entry.host_method == 3)
	{
		options.ip = entry.mitm_ip;
		options.port = entry.mitm_port;
		options.session = entry.mitm_session;
	}
	else
	{
		options.ip = entry.ip;
		options.port = entry.port;
		options.session = entry.traversal_code;
	}
	
	auto coreInfo = coreList.find(entry.core_name);
	if (coreInfo != coreList.cend())
		options.core = coreInfo->second;
	else 	
		options.core = Utils::String::toLower(Utils::String::replace(entry.core_name, " ", "_"));

	options.emulator = entry.emulator;
	
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	GuiSettings* msgBox = new GuiSettings(mWindow, _("CONNECT TO NETPLAY"));
	msgBox->setSubTitle(entry.game_name);
	msgBox->setTag("popup");
	
	std::shared_ptr<TextComponent> ed = nullptr;

	msgBox->addEntry(_U("\uF144 ") + _("JOIN GAME"), false, [this, msgBox, entry, options, ed]
	{		
		LaunchGameOptions opts = options;
		ViewController::get()->launch(entry.fileData, opts);

		auto pthis = this;
		msgBox->close();
		delete pthis;
	});

	msgBox->addEntry(_U("\uF06E ") + _("WATCH GAME"), false, [this, msgBox, entry, options, ed]
	{
		LaunchGameOptions opts = options;
		opts.netPlayMode = NetPlayMode::SPECTATOR;
		ViewController::get()->launch(entry.fileData, opts);

		auto pthis = this;
		msgBox->close();
		delete pthis;
	});
	
	mWindow->pushGui(msgBox);
}

bool GuiNetPlay::coreExists(FileData* file, std::string core_name)
{
	if (file == nullptr)
		return false;

	if (core_name.empty())
		return false;

	std::string coreName;

	auto coreInfo = coreList.find(core_name);
	if (coreInfo != coreList.cend())
		coreName = coreInfo->second;
	else
		coreName = Utils::String::toLower(Utils::String::replace(core_name, " ", "_"));

	for (auto& emul : file->getSourceFileData()->getSystem()->getEmulators())
		for (auto& core : emul.cores)
			if (core.name == coreName)
				return true;

	return false;
}

bool GuiNetPlay::hitTest(int x, int y, Transform4x4f& parentTransform, std::vector<GuiComponent*>* pResult)
{
	if (pResult) pResult->push_back(this); // Always return this as it's a fake fullscreen, so we always have click events
	GuiComponent::hitTest(x, y, parentTransform, pResult);
	return true;
}


bool GuiNetPlay::onMouseClick(int button, bool pressed, int x, int y)
{
	if (pressed && button == 1 && !mBackground.isMouseOver())
	{
		delete this;
		return true;
	}

	return (button == 1);
}