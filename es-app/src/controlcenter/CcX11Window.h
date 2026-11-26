#pragma once

#include <SDL.h>
#include <vector>

namespace CcX11Window
{
	void shapeRounded(SDL_Window* window, int x, int y, int w, int h, int radius);
	void activate(SDL_Window* window);
	bool captureBehind(SDL_Window* window, std::vector<unsigned char>& rgba, int& width, int& height);
}
