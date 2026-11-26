#include "controlcenter/CcX11Window.h"

#include <SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <algorithm>
#include <cmath>
#include <vector>

static const long PAGER_SOURCE = 2;

static bool x11Handles(SDL_Window* window, Display*& display, ::Window& xwindow)
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_X11)
		return false;

	display = info.info.x11.display;
	xwindow = info.info.x11.window;
	return true;
}

void CcX11Window::shapeRounded(SDL_Window* window, int x, int y, int w, int h, int radius)
{
	Display* display;
	::Window xwindow;
	if (!x11Handles(window, display, xwindow) || w <= 0 || h <= 0)
		return;

	radius = std::max(0, std::min(radius, std::min(w, h) / 2));

	std::vector<XRectangle> rects;
	rects.push_back({ (short)x, (short)(y + radius), (unsigned short)w, (unsigned short)std::max(0, h - 2 * radius) });
	for (int row = 0; row < radius; row++)
	{
		float dy = radius - row - 0.5f;
		int inset = radius - (int)std::sqrt(radius * radius - dy * dy);
		unsigned short width = (unsigned short)std::max(0, w - 2 * inset);
		rects.push_back({ (short)(x + inset), (short)(y + row), width, 1 });
		rects.push_back({ (short)(x + inset), (short)(y + h - 1 - row), width, 1 });
	}

	XShapeCombineRectangles(display, xwindow, ShapeBounding, 0, 0, rects.data(), (int)rects.size(), ShapeSet, Unsorted);
	XFlush(display);
}

void CcX11Window::activate(SDL_Window* window)
{
	Display* display;
	::Window xwindow;
	if (!x11Handles(window, display, xwindow))
		return;

	XEvent event = {};
	event.xclient.type = ClientMessage;
	event.xclient.window = xwindow;
	event.xclient.message_type = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
	event.xclient.format = 32;
	event.xclient.data.l[0] = PAGER_SOURCE;
	event.xclient.data.l[1] = CurrentTime;

	XRaiseWindow(display, xwindow);
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
	XFlush(display);
}

bool CcX11Window::captureBehind(SDL_Window* window, std::vector<unsigned char>& rgba, int& width, int& height)
{
	Display* display;
	::Window xwindow;
	if (!x11Handles(window, display, xwindow))
		return false;

	int x, y;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_GetWindowSize(window, &width, &height);
	if (width <= 0 || height <= 0)
		return false;

	XImage* image = XGetImage(display, DefaultRootWindow(display), x, y, width, height, AllPlanes, ZPixmap);
	if (image == nullptr)
		return false;

	rgba.resize((size_t)width * height * 4);
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			unsigned long pixel = XGetPixel(image, col, row);
			unsigned char* out = &rgba[((size_t)row * width + col) * 4];
			out[0] = (pixel & image->red_mask) >> __builtin_ctzl(image->red_mask);
			out[1] = (pixel & image->green_mask) >> __builtin_ctzl(image->green_mask);
			out[2] = (pixel & image->blue_mask) >> __builtin_ctzl(image->blue_mask);
			out[3] = 0xFF;
		}
	}

	XDestroyImage(image);
	return true;
}
