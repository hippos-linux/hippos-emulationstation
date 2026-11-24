#pragma once

#include <functional>

class Window;
class FileData;

void startNetplayHost(Window* window, FileData* game, const std::function<void()>& onHosting);
void openNetplayHostPicker(Window* window, const std::function<void()>& onHosting);
