#include "guis/GuiInstallPartitionSelect.h"

#include "ApiSystem.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiInstall.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"
#include "utils/StringUtil.h"
#include "LocaleES.h"

GuiInstallPartitionSelect::GuiInstallPartitionSelect(Window* window, std::string disk, std::string mode)
	: GuiComponent(window), mDisk(disk), mMode(mode),
	  mMenu(window, mode == "partition" ? _("CHOOSE PARTITION").c_str() : _("CHOOSE FREE SPACE").c_str())
{
	addChild(&mMenu);

	auto* api = ApiSystem::getInstance();

	// Target: partition or free space
	std::vector<std::string> targets = (mode == "partition")
		? api->getInstallDiskTargets(disk)
		: api->getInstallDiskFreeSpace(disk);

	std::string targetLabel = (mode == "partition") ? _("TARGET PARTITION") : _("FREE SPACE REGION");

	moptionsTarget = std::make_shared<OptionListComponent<std::string>>(window, targetLabel, false);
	moptionsTarget->add(_("SELECT"), "", true);
	for (auto& entry : targets)
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
			moptionsTarget->add(vname, tokens.at(0), false);
		}
	}
	mMenu.addWithLabel(targetLabel, moptionsTarget);

	// EFI partition picker
	std::vector<std::string> efiParts = api->getInstallDiskEfiPartitions(disk);
	moptionsEfi = std::make_shared<OptionListComponent<std::string>>(window, _("EFI PARTITION"), false);
	moptionsEfi->add(_("SELECT"), "", true);
	for (auto& entry : efiParts)
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
			moptionsEfi->add(vname, tokens.at(0), false);
		}
	}
	mMenu.addWithLabel(_("EFI PARTITION"), moptionsEfi);

	if (targets.empty())
	{
		std::string msg = (mode == "partition")
			? _("NO SUITABLE PARTITIONS FOUND (need ≥16GiB non-EFI)")
			: _("NO FREE SPACE FOUND (need ≥16GiB unallocated)");
		mMenu.addButton(msg, "back", [&] { delete this; });
	}
	else
	{
		moptionsValidation = std::make_shared<SwitchComponent>(mWindow);
		mMenu.addWithLabel(_("ARE YOU SURE?"), moptionsValidation);

		mMenu.addButton(_("INSTALL"), "install", std::bind(&GuiInstallPartitionSelect::start, this));
		mMenu.addButton(_("BACK"),    "back",    [&] { delete this; });
	}

	if (Renderer::ScreenSettings::fullScreenMenus())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.1f);
}

void GuiInstallPartitionSelect::start()
{
	if (!moptionsTarget->hasSelection() || moptionsTarget->getSelected().empty() ||
	    !moptionsEfi->hasSelection()    || moptionsEfi->getSelected().empty()    ||
	    !moptionsValidation->getState())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("INVALID PARAMETERS")));
		return;
	}

	std::string target = moptionsTarget->getSelected();
	std::string efi    = moptionsEfi->getSelected();
	std::string disk   = mDisk;
	std::string mode   = mMode;
	Window*     win    = mWindow;

	auto op = (mode == "partition")
		? std::function<std::pair<std::string,int>(BusyComponent*)>(
			[target, efi](BusyComponent* ui) {
				return ApiSystem::getInstance()->installToPartition(ui, target, efi);
			})
		: std::function<std::pair<std::string,int>(BusyComponent*)>(
			[disk, target, efi](BusyComponent* ui) {
				return ApiSystem::getInstance()->installInFreeSpace(ui, disk, target, efi);
			});

	win->pushGui(new GuiInstall(win, op));
	delete this;
}

bool GuiInstallPartitionSelect::input(InputConfig* config, Input input)
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

std::vector<HelpPrompt> GuiInstallPartitionSelect::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}
