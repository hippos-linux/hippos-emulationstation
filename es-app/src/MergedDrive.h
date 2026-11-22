#pragma once
#ifndef ES_APP_MERGED_DRIVE_H
#define ES_APP_MERGED_DRIVE_H

#include <optional>
#include <string>
#include <unordered_set>

namespace MergedDrive
{
	bool isOnMergedDrive(const std::string& path);
	std::optional<std::unordered_set<std::string>> internalFileNames(const std::string& dir);
}

#endif // ES_APP_MERGED_DRIVE_H
