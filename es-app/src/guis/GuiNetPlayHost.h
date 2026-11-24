#pragma once

#include <functional>

class Window;
class FileData;
struct LaunchGameOptions;

void startNetplayHost(Window* window, FileData* game);
void launchFromNetplayLobby(Window* window, FileData* game, LaunchGameOptions options);
void openNetplayHostPicker(Window* window, const std::function<void(FileData*)>& onGameChosen);
