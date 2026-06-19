#include "guis/GuiUpdate.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "Window.h"
#include <string>
#include "Log.h"
#include "Settings.h"
#include "ApiSystem.h"
#include "utils/Platform.h"
#include "LocaleES.h"
#include "components/AsyncNotificationComponent.h"

GuiUpdateState::State GuiUpdate::state = GuiUpdateState::State::NO_UPDATE;

class ThreadedUpdater
{
public:
	// System update
	ThreadedUpdater(Window* window)
		: mWindow(window), mMode(Mode::System) {}

	// Single emulator update
	static ThreadedUpdater* forEmulator(Window* window, const std::string& name)
	{
		auto* t = new ThreadedUpdater(window, Mode::Emulator);
		t->mEmulatorName = name;
		return t;
	}

	// All emulators update
	static ThreadedUpdater* forAllEmulators(Window* window)
	{
		return new ThreadedUpdater(window, Mode::AllEmulators);
	}

	// Frontend update
	static ThreadedUpdater* forFrontend(Window* window, const std::string& name)
	{
		auto* t = new ThreadedUpdater(window, Mode::Frontend);
		t->mFrontendName = name;
		return t;
	}

	// Patches update
	static ThreadedUpdater* forPatches(Window* window)
	{
		return new ThreadedUpdater(window, Mode::Patches);
	}

	void start()
	{
		GuiUpdate::state = GuiUpdateState::State::UPDATER_RUNNING;
		mWndNotification = mWindow->createAsyncNotificationComponent();

		switch (mMode)
		{
			case Mode::System:
			{
				auto label = Utils::String::format(_("UPDATING %s").c_str(), ApiSystem::getInstance()->getApplicationName().c_str());
				mWndNotification->updateTitle(_U(" ") + label);
				break;
			}
			case Mode::Emulator:
				mWndNotification->updateTitle(_U(" ") + _("UPDATING") + " " + mEmulatorName);
				break;
			case Mode::AllEmulators:
				mWndNotification->updateTitle(_U(" ") + _("UPDATING EMULATORS"));
				break;
			case Mode::Frontend:
				mWndNotification->updateTitle(_U(" ") + _("UPDATING EMULATIONSTATION"));
				break;
			case Mode::Patches:
				mWndNotification->updateTitle(_U(" ") + _("APPLYING PATCHES"));
				break;
		}

		mHandle = new std::thread(&ThreadedUpdater::threadUpdate, this);
	}

	~ThreadedUpdater()
	{
		if (mWndNotification)
		{
			mWndNotification->close();
			mWndNotification = nullptr;
		}
	}

	void threadUpdate()
	{
		std::pair<std::string, int> updateStatus = { "", 0 };

		switch (mMode)
		{
			case Mode::System:
				updateStatus = ApiSystem::getInstance()->updateSystem([this](const std::string info)
				{
					auto pos = info.find(">>>");
					if (pos != std::string::npos)
					{
						std::string percent(info.substr(pos));
						percent = Utils::String::replace(percent, ">", "");
						percent = Utils::String::replace(percent, "%", "");
						percent = Utils::String::replace(percent, " ", "");
						int value = atoi(percent.c_str());
						std::string text(info.substr(0, pos));
						text = Utils::String::trim(text);
						mWndNotification->updatePercent(value);
						mWndNotification->updateText(text);
					}
					else
					{
						mWndNotification->updatePercent(-1);
						mWndNotification->updateText(info);
					}
				});
				break;

			case Mode::Emulator:
				updateStatus = ApiSystem::getInstance()->updateEmulator(mEmulatorName, [this](const std::string info)
				{
					mWndNotification->updatePercent(-1);
					mWndNotification->updateText(info);
				});
				break;

			case Mode::AllEmulators:
				updateStatus = ApiSystem::getInstance()->updateEmulators([this](const std::string info)
				{
					mWndNotification->updatePercent(-1);
					mWndNotification->updateText(info);
				});
				break;

			case Mode::Frontend:
				updateStatus = ApiSystem::getInstance()->updateFrontend(mFrontendName, [this](const std::string info)
				{
					mWndNotification->updatePercent(-1);
					mWndNotification->updateText(info);
				});
				break;

			case Mode::Patches:
				updateStatus = ApiSystem::getInstance()->applyPatches([this](const std::string info)
				{
					mWndNotification->updatePercent(-1);
					mWndNotification->updateText(info);
				});
				break;
		}

		if (updateStatus.second == 0)
		{
			GuiUpdate::state = (mMode == Mode::System)
				? GuiUpdateState::State::UPDATE_READY
				: GuiUpdateState::State::NO_UPDATE;

			if (mMode == Mode::System)
			{
				mWndNotification->updateTitle(_U(" ") + _("UPDATE IS READY"));
				mWndNotification->updateText(_("REBOOT TO APPLY"));
			}
			else if (mMode == Mode::Frontend)
			{
				mWndNotification->updateTitle(_U(" ") + _("EMULATIONSTATION UPDATED"));
				mWndNotification->updateText(_("RESTART TO APPLY"));
			}
			else if (mMode == Mode::Patches)
			{
				mWndNotification->updateTitle(_U(" ") + _("PATCHES APPLIED"));
				mWndNotification->updateText(_("PATCHES APPLIED SUCCESSFULLY"));
			}
			else
			{
				mWndNotification->updateTitle(_U(" ") + _("EMULATOR UPDATED"));
				mWndNotification->updateText(mEmulatorName.empty() ? _("ALL EMULATORS UP TO DATE") : mEmulatorName);
			}

			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::hours(12));
		}
		else
		{
			GuiUpdate::state = GuiUpdateState::State::NO_UPDATE;
			std::string error = _("AN ERROR OCCURRED") + std::string(": ") + updateStatus.first;
			mWindow->displayNotificationMessage(error);
		}

		delete this;
	}

private:
	enum class Mode { System, Emulator, AllEmulators, Frontend, Patches };

	ThreadedUpdater(Window* window, Mode mode)
		: mWindow(window), mMode(mode), mWndNotification(nullptr), mHandle(nullptr) {}

	std::thread*				mHandle;
	AsyncNotificationComponent* mWndNotification;
	Window*						mWindow;
	Mode						mMode;
	std::string					mEmulatorName;
	std::string					mFrontendName;
};


GuiUpdate::GuiUpdate(Window* window) : GuiComponent(window), mBusyAnim(window)
{
	LOG(LogInfo) << "Starting GuiUpdate";

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	mState = 0;
	mLoading = true;
	mHasRootfsUpdate = false;
	mHasEmulatorUpdates = false;
	mHasFrontendUpdate = false;
	mHasPatchUpdates = false;
	mPingHandle = new std::thread(&GuiUpdate::threadPing, this);
	mBusyAnim.setSize(mSize);
}

GuiUpdate::~GuiUpdate()
{
	mPingHandle->join();
	delete mPingHandle;
}

void GuiUpdate::threadPing()
{
	if (!ApiSystem::getInstance()->ping())
	{
		onPingError();
		return;
	}

	std::vector<std::string> rootfsMsgs;
	mHasRootfsUpdate = ApiSystem::getInstance()->canUpdate(rootfsMsgs);
	if (mHasRootfsUpdate && rootfsMsgs.size() == 1)
		mUpdateVersion = rootfsMsgs[0];

	// Only lines with "→" are actual pending updates (hippos-upgrade filters these now)
	mEmulatorUpdates = ApiSystem::getInstance()->listEmulatorUpdates();
	mHasEmulatorUpdates = !mEmulatorUpdates.empty();

	auto frontendUpdates = ApiSystem::getInstance()->listFrontendUpdates();
	if (!frontendUpdates.empty())
	{
		mHasFrontendUpdate = true;
		// Extract the version string from "emulationstation\told → new"
		auto& line = frontendUpdates[0];
		auto tab = line.find('\t');
		mFrontendUpdateName = (tab != std::string::npos) ? line.substr(0, tab) : "emulationstation";
	}

	mPatchUpdates = ApiSystem::getInstance()->listPatches();
	mHasPatchUpdates = !mPatchUpdates.empty();

	if (!mHasRootfsUpdate && !mHasEmulatorUpdates && !mHasFrontendUpdate && !mHasPatchUpdates)
		onNoUpdateAvailable();
	else
		onUpdateAvailable();
}

void GuiUpdate::onUpdateAvailable()
{
	mLoading = false;
	LOG(LogInfo) << "GuiUpdate : Update available (rootfs=" << mHasRootfsUpdate
		<< " emulators=" << mHasEmulatorUpdates
		<< " frontend=" << mHasFrontendUpdate
		<< " patches=" << mHasPatchUpdates << ")";
	mState = 9; // show update list
}

void GuiUpdate::onNoUpdateAvailable()
{
	mLoading = false;
	LOG(LogInfo) << "GuiUpdate : No update available";
	mState = 6;
}

void GuiUpdate::onPingError()
{
	LOG(LogError) << "GuiUpdate : Ping failed";
	mLoading = false;
	mState = 3;
}

void GuiUpdate::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mLoading)
		mBusyAnim.update(deltaTime);

	Window* window = mWindow;

	switch (mState)
	{
		case 9: // show individual update entries
		{
			mState = 0;

			auto* updateList = new GuiSettings(window, _("AVAILABLE UPDATES").c_str());

			if (mHasRootfsUpdate)
			{
				std::string label = _("SYSTEM");
				if (!mUpdateVersion.empty())
					label += "  →  " + mUpdateVersion;

				updateList->addEntry(label.c_str(), false, [this, window]
				{
					std::string msg;
					std::string versionExtra = ApiSystem::getInstance()->getVersion(true);
					if (!mUpdateVersion.empty())
					{
						if (versionExtra == "none")
							msg = Utils::String::format(_("YOU ARE CURRENTLY USING VERSION %s\nDO YOU WANT TO UPDATE TO VERSION %s?").c_str(),
								ApiSystem::getInstance()->getVersion().c_str(), mUpdateVersion.c_str());
						else
							msg = Utils::String::format(_("UNOFFICIAL SYSTEM MODIFICATIONS DETECTED.\nUPGRADING COULD BREAK YOUR SYSTEM.\nDO YOU WANT TO UPDATE TO VERSION %s?").c_str(),
								mUpdateVersion.c_str());
					}
					else
					{
						msg = _("REALLY UPDATE?");
					}

					window->pushGui(new GuiMsgBox(window, msg,
						_("YES"), [this]
						{
							auto* t = new ThreadedUpdater(mWindow);
							t->start();
						},
						_("NO"), nullptr));
				});
			}

			if (mHasFrontendUpdate)
			{
				std::string frontendName = mFrontendUpdateName;
				updateList->addEntry(_("EMULATIONSTATION"), false, [this, frontendName]
				{
					auto* t = ThreadedUpdater::forFrontend(mWindow, frontendName);
					t->start();
				});
			}

			// One entry per emulator — show a clean display name
			for (auto& line : mEmulatorUpdates)
			{
				auto tab = line.find('\t');
				std::string emuName = (tab != std::string::npos) ? line.substr(0, tab) : line;

				// Build display name: capitalise each hyphen/underscore-separated word
				std::string label;
				bool newWord = true;
				for (char c : emuName)
				{
					if (c == '-' || c == '_')
					{
						label += ' ';
						newWord = true;
					}
					else
					{
						label += newWord ? (char)toupper(c) : c;
						newWord = false;
					}
				}

				updateList->addEntry(label.c_str(), false, [this, emuName]
				{
					auto* t = ThreadedUpdater::forEmulator(mWindow, emuName);
					t->start();
				});
			}

			if (mHasPatchUpdates)
			{
				std::string label = _("PATCHES");
				label += "  (" + std::to_string(mPatchUpdates.size()) + ")";
				updateList->addEntry(label.c_str(), false, [this]
				{
					auto* t = ThreadedUpdater::forPatches(mWindow);
					t->start();
				});
			}

			window->pushGui(updateList);
			mState = -1;
		}
		break;

		case 3:
			mState = 0;
			window->pushGui(new GuiMsgBox(window, _("NETWORK CONNECTION NEEDED"), _("OK"), [this]
			{
				mState = -1;
			}));
			break;

		case 6:
			mState = 0;
			window->pushGui(new GuiMsgBox(window, _("NO UPDATE AVAILABLE"), _("OK"), [this]
			{
				mState = -1;
			}));
			break;

		case -1:
			delete this;
			break;
	}
}

void GuiUpdate::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	renderChildren(trans);

	Renderer::setMatrix(trans);
	Renderer::drawRect(0.f, 0.f, mSize.x(), mSize.y(), 0x00000011);

	if (mLoading)
		mBusyAnim.render(trans);
}

bool GuiUpdate::input(InputConfig* config, Input input)
{
	return false;
}

std::vector<HelpPrompt> GuiUpdate::getHelpPrompts()
{
	return std::vector<HelpPrompt>();
}
