#include "HipposLobby.h"

#include "FileData.h"
#include "SystemConf.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <map>

static const std::map<std::string, int> challengeEmulatorPorts = { { "flycast", 19713 } };

std::string HipposLobby::url(const std::string& path)
{
	std::string base = SystemConf::getInstance()->get("global.netplay.hipposlobby");
	if (base.empty())
		return "";

	if (base.back() == '/')
		base.pop_back();

	return base + path;
}

bool HipposLobby::takesChallenges(const std::string& emulator)
{
	return challengeEmulatorPorts.find(emulator) != challengeEmulatorPorts.cend();
}

std::string HipposLobby::challengeRoomJson(FileData* game)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	writer.StartObject();
	writer.Key("port");
	writer.Int(challengeEmulatorPorts.at(game->getEmulator()));
	writer.Key("emulator");
	writer.String(game->getEmulator().c_str());
	writer.Key("core_name");
	writer.String(game->getCore().c_str());
	writer.Key("game_name");
	writer.String(game->getName().c_str());
	writer.Key("game_crc");
	writer.String(game->getMetadata(MetaDataId::Crc32).c_str());
	writer.Key("username");
	writer.String(SystemConf::getInstance()->get("global.netplay.nickname").c_str());
	writer.Key("mode");
	writer.String("challenge");
	writer.EndObject();
	return buffer.GetString();
}
