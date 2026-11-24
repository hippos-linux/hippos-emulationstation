#pragma once

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/BusyComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include <memory>
#include <unordered_set>
#include <vector>
#include <thread>
#include <atomic>

class HttpReq;
class FileData;

// from RetroArch network/netplay
#define NETPLAY_NICK_LEN         32
#define NETPLAY_HOST_STR_LEN     32
#define NETPLAY_HOST_LONGSTR_LEN 256
#define DISCOVERY_QUERY_MAGIC    0x52414E51 /* RANQ */
#define DISCOVERY_RESPONSE_MAGIC 0x52414E53 /* RANS */

struct ad_packet
{
	uint32_t header;
	int32_t  content_crc;
	int32_t  port;
	uint32_t has_password;
	char     nick[NETPLAY_NICK_LEN];
	char     frontend[NETPLAY_HOST_STR_LEN];
	char     core[NETPLAY_HOST_STR_LEN];
	char     core_version[NETPLAY_HOST_STR_LEN];
	char     retroarch_version[NETPLAY_HOST_STR_LEN];
	char     content[NETPLAY_HOST_LONGSTR_LEN];
	char     subsystem_name[NETPLAY_HOST_LONGSTR_LEN];
};

enum class LobbyKind
{
	Libretro,
	Hippos,
	Dolphin
};

struct LobbySource
{
	std::unique_ptr<HttpReq> request;
	std::string content;
	LobbyKind kind = LobbyKind::Libretro;
};

struct LobbyAppEntry
{
	FileData*   fileData = nullptr;
	std::string username;
	std::string game_crc;
	std::string mitm_ip;
	std::string subsystem_name;
	std::string frontend;
	std::string created;
	std::string core_version;
	std::string ip;
	std::string updated;
	std::string country;
	int         host_method = 0;
	bool		has_password = false;
	std::string game_name;
	bool		has_spectate_password = false;
	std::string core_name;
	std::string emulator;
	int         mitm_port = 0;
	std::string mitm_session;
	std::string traversal_code;
	std::string room_id;
	bool		hippos = false;
	bool		challenge = false;
	bool		own = false;
	bool		fixed = false;
	std::string retroarch_version;
	int         port = 0;

	bool		isCrcValid = false;
	bool		coreExists = false;
};


class NetPlayChat;
class NetPlayChallenge;

class GuiNetPlay : public GuiComponent 
{
public:
	static void open(Window* window, FileData* challengeGame = nullptr);

	GuiNetPlay(Window* window);
	~GuiNetPlay();

	void update(int deltaTime) override;
	void render(const Transform4x4f &parentTrans) override;
	void onSizeChanged() override;
	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

	bool hitTest(int x, int y, Transform4x4f& parentTransform, std::vector<GuiComponent*>* pResult = nullptr) override;
	bool onMouseClick(int button, bool pressed, int x, int y);

private:
	void startRequest();
	
	void populateFromLobbies(std::vector<std::pair<std::string, LobbyKind>> responses);
	void populateFromJson(const std::string& json, bool hipposLobby, std::vector<LobbyAppEntry>& entries);
	void populateFromDolphinJson(const std::string& json, std::vector<LobbyAppEntry>& entries);
	void mergeHipposRetroArchRooms(std::vector<LobbyAppEntry>& entries);
	bool populateFromLan();

	bool populateList();

	void launchGame(LobbyAppEntry entry);
	void lanLobbyRequest();
	void chooseChatRoom();
	void hostGame(FileData* game);
	void chooseChallengeOptions(FileData* game);
	void startChallenge(FileData* game, bool autoAccept);
	void confirmStopChallenge();
	bool updateHostedChallenge(int deltaTime);
	void askToAcceptChallenger(const std::string& username);
	void changeNickname();
	void toggleHipposOnly();
	std::string listFilterSubtitle() const;

	FileData* getFileData(const std::string& gameInfo, bool crc = true, const std::string& coreName = "");
	bool coreExists(FileData* file, std::string core_name);

	int								mLanLobbySocket;
	int								mLanLobbySocketTimeout;
	int								mRefreshDelay;

	NinePatchComponent				mBackground;
	ComponentGrid					mGrid;

	std::shared_ptr<TextComponent>	mTitle;
	std::shared_ptr<TextComponent>	mSubtitle;

	std::shared_ptr<ComponentList>	mList;
	std::shared_ptr<NetPlayChat>	mChat;

	std::shared_ptr<ComponentGrid>	mHeaderGrid;
	std::shared_ptr<ComponentGrid>	mButtonGrid;


	BusyComponent					mBusyAnim;

	std::vector<LobbySource>		mLobbySources;
	std::unique_ptr<NetPlayChallenge>	mHostedChallenge;
	std::string						mListedChallengeRoom;

	std::vector<LobbyAppEntry>		mLanEntries;
	std::vector<LobbyAppEntry>		mLobbyEntries;

	std::thread*					mPopulateThread;
	std::atomic<bool>				mThreadFinished;
};
