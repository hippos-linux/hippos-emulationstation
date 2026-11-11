#pragma once

#include "Window.h"
#include <map>
#include <mutex>
#include <thread>

class AsyncNotificationComponent;

namespace httplib
{
	class Server;
}

class HttpServerThread
{
public:
	HttpServerThread(Window * window);
	virtual ~HttpServerThread();

	static std::string getMimeType(const std::string &path);

private:
	Window*			mWindow;
	bool			mRunning;
	bool			mFirstRun;
	std::thread*	mThread;

	httplib::Server* mHttpServer;

	std::mutex mProgressLock;
	std::map<std::string, AsyncNotificationComponent*> mProgress;

	void run();
};


