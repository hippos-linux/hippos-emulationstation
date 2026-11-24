#include "guis/GuiNetPlayHost.h"

#include "ApiSystem.h"
#include "FileData.h"
#include "LocaleES.h"
#include "SystemData.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#include <algorithm>

void startNetplayHost(Window* window, FileData* game)
{
	if (ApiSystem::getInstance()->getIpAddress() == "NOT CONNECTED")
	{
		window->pushGui(new GuiMsgBox(window, _("YOU ARE NOT CONNECTED TO A NETWORK"), _("OK"), nullptr));
		return;
	}

	game->checkCrc32();
	LaunchGameOptions options;
	options.netPlayMode = SERVER;
	ViewController::get()->launch(game, options);
}

static void openNetplayGamePicker(Window* window, SystemData* system)
{
	auto games = system->getRootFolder()->getFilesRecursive(GAME, true, system);
	std::sort(games.begin(), games.end(), [](FileData* a, FileData* b) { return Utils::String::compareIgnoreCase(a->getName(), b->getName()) < 0; });

	auto picker = new GuiSettings(window, _("HOST A NETPLAY GAME"));
	picker->setSubTitle(system->getFullName());

	for (auto game : games)
	{
		if (!game->isNetplaySupported())
			continue;

		picker->addEntry(game->getName(), false, [window, picker, game]
		{
			picker->close();
			startNetplayHost(window, game);
		});
	}

	window->pushGui(picker);
}

void openNetplayHostPicker(Window* window)
{
	auto picker = new GuiSettings(window, _("HOST A NETPLAY GAME"));
	picker->setSubTitle(_("Choose a system"));

	for (auto system : SystemData::sSystemVector)
	{
		if (system->isCollection() || system->isGroupChildSystem() || !system->isNetplaySupported() || system->getGameCount() == 0)
			continue;

		picker->addEntry(system->getFullName(), true, [window, system] { openNetplayGamePicker(window, system); });
	}

	window->pushGui(picker);
}
