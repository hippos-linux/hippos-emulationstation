#include "DolphinGameId.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <cctype>
#include <fstream>
#include <map>

static const std::map<std::string, std::streamoff> gameIdOffsets =
{
	{ ".iso", 0x0 },
	{ ".gcm", 0x0 },
	{ ".ciso", 0x8000 },
	{ ".wbfs", 0x200 },
	{ ".rvz", 0x58 },
	{ ".wia", 0x58 },
};

std::string readDolphinGameId(const std::string& path)
{
	auto offset = gameIdOffsets.find(Utils::String::toLower(Utils::FileSystem::getExtension(path)));
	if (offset == gameIdOffsets.cend())
		return "";

	std::ifstream file(path, std::ios::binary);
	char id[6];
	if (!file.seekg(offset->second) || !file.read(id, sizeof(id)))
		return "";

	for (char ch : id)
		if (!std::isupper((unsigned char)ch) && !std::isdigit((unsigned char)ch))
			return "";

	return std::string(id, sizeof(id));
}
