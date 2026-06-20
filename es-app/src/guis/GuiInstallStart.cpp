#include "guis/GuiInstallStart.h"

#include "ApiSystem.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiInstall.h"
#include "guis/GuiInstallPartitionSelect.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"
#include "utils/StringUtil.h"
#include "LocaleES.h"

GuiInstallStart::GuiInstallStart(Window* window) : GuiComponent(window),
	mMenu(window, _("INSTALL ON A NEW DISK").c_str())
{
	addChild(&mMenu);

	std::vector<std::string> availableStorage      = ApiSystem::getInstance()->getAvailableInstallDevices();
	std::vector<std::string> availableArchitecture = ApiSystem::getInstance()->getAvailableInstallArchitectures();
	std::string runningBoard = ApiSystem::getInstance()->getRunningBoard();

	bool installationPossible = !availableArchitecture.empty();

	if (installationPossible)
	{
		// Target device
		moptionsStorage = std::make_shared<OptionListComponent<std::string>>(window, _("TARGET DEVICE"), false);
		moptionsStorage->add(_("SELECT"), "", true);
		for (auto& entry : availableStorage)
		{
			std::vector<std::string> tokens = Utils::String::split(entry, ' ');
			if (tokens.size() >= 2)
			{
				std::string vname;
				for (unsigned int i = 1; i < tokens.size(); i++)
				{
					if (i > 1) vname += " ";
					vname += tokens.at(i);
				}
				moptionsStorage->add(vname, tokens.at(0), false);
			}
		}
		mMenu.addWithLabel(_("TARGET DEVICE"), moptionsStorage);

		// Install mode
		moptionsMode = std::make_shared<OptionListComponent<std::string>>(window, _("INSTALL MODE"), false);
		moptionsMode->add(_("FULL DISK (wipe)"),              "full",      true);
		moptionsMode->add(_("EXISTING PARTITION"),            "partition", false);
		moptionsMode->add(_("NEW PARTITION IN FREE SPACE"),   "freespace", false);
		mMenu.addWithLabel(_("INSTALL MODE"), moptionsMode);

		// Architecture
		moptionsArchitecture = std::make_shared<OptionListComponent<std::string>>(window, _("TARGET ARCHITECTURE"), false);
		moptionsArchitecture->add(_("SELECT"), "", false);
		for (auto& arch : availableArchitecture)
			moptionsArchitecture->add(arch, arch, arch == runningBoard);
		if (!moptionsArchitecture->hasSelection())
			moptionsArchitecture->selectFirstItem();
		mMenu.addWithLabel(_("TARGET ARCHITECTURE"), moptionsArchitecture);

		moptionsValidation = std::make_shared<SwitchComponent>(mWindow);
		mMenu.addWithLabel(_("ARE YOU SURE?"), moptionsValidation);

		mMenu.addButton(_("NEXT"),  "next",  std::bind(&GuiInstallStart::start, this));
		mMenu.addButton(_("BACK"),  "back",  [&] { delete this; });
	}
	else
		mMenu.addButton(_("NETWORK REQUIRED"), "back", [&] { delete this; });

	if (Renderer::ScreenSettings::fullScreenMenus())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.1f);
}

void GuiInstallStart::start()
{
	if (!moptionsStorage->hasSelection() || moptionsStorage->getSelected().empty() ||
	    !moptionsMode->hasSelection()    || !moptionsValidation->getState())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("INVALID PARAMETERS")));
		return;
	}

	std::string disk = moptionsStorage->getSelected();
	std::string mode = moptionsMode->getSelected();
	std::string arch = moptionsArchitecture->getSelected();

	if (mode == "full")
	{
		if (!moptionsArchitecture->hasSelection() || arch.empty())
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, _("INVALID PARAMETERS")));
			return;
		}

		// Build partition warning
		std::vector<std::string> parts = ApiSystem::getInstance()->getInstallDiskAllPartitions(disk);
		std::string warning = _("WARNING: This will DESTROY ALL DATA on") + " " + disk + ".\n\n";
		if (!parts.empty())
		{
			warning += _("Partitions that will be erased:") + "\n";
			for (auto& p : parts)
			{
				std::vector<std::string> tokens = Utils::String::split(p, ' ');
				std::string desc = tokens.size() >= 2 ? p : p;
				warning += "  " + desc + "\n";
			}
			warning += "\n";
		}
		warning += _("Continue?");

		Window* win  = mWindow;
		std::string  d = disk, a = arch;
		mWindow->pushGui(new GuiMsgBox(mWindow, warning,
			_("YES"), [win, d, a] {
				win->pushGui(new GuiInstall(win, d, a));
			},
			_("NO"), nullptr
		));
		delete this;
	}
	else
	{
		// Partition or free-space: push selection screen
		mWindow->pushGui(new GuiInstallPartitionSelect(mWindow, disk, mode));
		delete this;
	}
}

bool GuiInstallStart::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if (consumed) return true;

	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		delete this;
		return true;
	}

	if (config->isMappedTo("start", input) && input.value != 0)
	{
		Window* window = mWindow;
		while (window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();
	}

	return false;
}

std::vector<HelpPrompt> GuiInstallStart::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}
