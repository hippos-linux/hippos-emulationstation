#pragma once

#include <functional>

class Window;
class FileData;

bool startNetplayHost(Window* window, FileData* game);
void openNetplayHostPicker(Window* window, const std::function<void()>& onHosting);
