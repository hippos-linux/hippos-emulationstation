#include "MergedDrive.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <map>
#include <mutex>

#if !defined(WIN32)
#include <sys/xattr.h>
#endif

namespace MergedDrive
{
#if defined(WIN32)
	bool isOnMergedDrive(const std::string& path) { return false; }
	std::optional<std::unordered_set<std::string>> internalFileNames(const std::string& dir) { return std::nullopt; }
#else
	struct Pool
	{
		std::string mountPoint;
		std::string internalBranch;
	};

	static std::string readAttribute(const std::string& path, const char* name)
	{
		char value[4096];
		ssize_t size = getxattr(path.c_str(), name, value, sizeof(value));
		return size > 0 ? std::string(value, size) : std::string();
	}

	static Pool findPool(const std::string& dir)
	{
		for (std::string current = dir; !current.empty() && current != "/"; current = Utils::FileSystem::getParent(current))
		{
			std::string branches = readAttribute(current + "/.mergerfs", "user.mergerfs.branches");
			if (branches.empty())
				continue;

			auto list = Utils::String::split(branches, ':');
			return Pool{ current, list[0].substr(0, list[0].find('=')) };
		}
		return Pool();
	}

	static Pool poolFor(const std::string& dir)
	{
		static std::mutex lock;
		static std::map<std::string, Pool> byDirectory;

		std::lock_guard<std::mutex> guard(lock);
		auto it = byDirectory.find(dir);
		if (it == byDirectory.cend())
			it = byDirectory.emplace(dir, findPool(dir)).first;

		return it->second;
	}

	bool isOnMergedDrive(const std::string& path)
	{
		Pool pool = poolFor(Utils::FileSystem::getParent(path));
		if (pool.internalBranch.empty())
			return false;

		return !Utils::FileSystem::exists(pool.internalBranch + path.substr(pool.mountPoint.size()));
	}

	std::optional<std::unordered_set<std::string>> internalFileNames(const std::string& dir)
	{
		Pool pool = poolFor(dir);
		if (pool.internalBranch.empty())
			return std::nullopt;

		std::unordered_set<std::string> names;
		for (const auto& file : Utils::FileSystem::getDirectoryFiles(pool.internalBranch + dir.substr(pool.mountPoint.size())))
			names.insert(Utils::FileSystem::getFileName(file.path));

		return names;
	}
#endif
}
