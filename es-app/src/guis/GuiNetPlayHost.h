#pragma once

#include <functional>

class Window;
class FileData;
struct LaunchGameOptions;

bool startNetplayHost(Window* window, FileData* game, bool listInLobby);
void closeNetplayMenus(Window* window);
void launchFromNetplayLobby(Window* window, FileData* game, const LaunchGameOptions& options);
void openNetplayHostPicker(Window* window, const std::function<void(FileData*)>& onGameChosen);
