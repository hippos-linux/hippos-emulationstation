#include "guis/GuiCloneStart.h"

#include "ApiSystem.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiClone.h"
#include "guis/GuiMsgBox.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "LocaleES.h"

GuiCloneStart::GuiCloneStart(Window* window) : GuiComponent(window),
	mMenu(window, _("CLONE TO EXTERNAL DRIVE").c_str())
{
	addChild(&mMenu);

	std::vector<std::string> hipposDisks = ApiSystem::getInstance()->getAvailableHipposDisks();
	std::vector<std::string> targetDisks = ApiSystem::getInstance()->getAvailableInstallDevices();

	bool possible = !hipposDisks.empty() && !targetDisks.empty();

	if (possible)
	{
		// Source: HippOS-labelled disks (installed drives)
		moptionsSource = std::make_shared<OptionListComponent<std::string>>(window, _("SOURCE DISK"), false);
		moptionsSource->add(_("SELECT"), "", true);
		for (auto& entry : hipposDisks)
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
				moptionsSource->add(vname, tokens.at(0), false);
			}
		}
		mMenu.addWithLabel(_("SOURCE DISK"), moptionsSource);

		// Target: any non-boot disk (the USB to clone onto)
		moptionsTarget = std::make_shared<OptionListComponent<std::string>>(window, _("TARGET DEVICE"), false);
		moptionsTarget->add(_("SELECT"), "", true);
		for (auto& entry : targetDisks)
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
		mMenu.addWithLabel(_("TARGET DEVICE"), moptionsTarget);

		moptionsValidation = std::make_shared<SwitchComponent>(mWindow);
		mMenu.addWithLabel(_("ARE YOU SURE?"), moptionsValidation);

		mMenu.addButton(_("CLONE"), "clone", std::bind(&GuiCloneStart::start, this));
		mMenu.addButton(_("BACK"), "back", [&] { delete this; });
	}
	else
	{
		std::string msg = hipposDisks.empty()
			? _("NO INSTALLED HIPPOS DISK FOUND")
			: _("NO TARGET DEVICE AVAILABLE");
		mMenu.addButton(msg, "back", [&] { delete this; });
	}

	if (Renderer::ScreenSettings::fullScreenMenus())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.1f);
}

void GuiCloneStart::start()
{
	if (moptionsSource->hasSelection() && moptionsSource->getSelected() != "" &&
		moptionsTarget->hasSelection() && moptionsTarget->getSelected() != "" &&
		moptionsValidation->getState())
	{
		std::string src = moptionsSource->getSelected();
		std::string tgt = moptionsTarget->getSelected();

		if (src == tgt)
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, _("SOURCE AND TARGET CANNOT BE THE SAME DISK")));
			return;
		}

		mWindow->pushGui(new GuiClone(mWindow, src, tgt));
		delete this;
	}
	else
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("INVALID PARAMETERS")));
	}
}

bool GuiCloneStart::input(InputConfig* config, Input input)
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

std::vector<HelpPrompt> GuiCloneStart::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}
