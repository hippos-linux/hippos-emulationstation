#pragma once

#include <string>

class FileData;

namespace HipposLobby
{
	std::string url(const std::string& path);
	bool takesChallenges(const std::string& emulator);
	std::string challengeRoomJson(FileData* game);
}
