#pragma once

#include <functional>

class Window;
class FileData;

void startNetplayHost(Window* window, FileData* game);
void openNetplayHostPicker(Window* window, const std::function<void(FileData*)>& onGameChosen);
