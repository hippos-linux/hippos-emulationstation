#pragma once

#include "GuiComponent.h"
#include "components/MenuComponent.h"

#include <string>

template<typename T>
class OptionListComponent;

class SwitchComponent;

class GuiInstallPartitionSelect : public GuiComponent
{
public:
	// mode: "partition" or "freespace"
	GuiInstallPartitionSelect(Window* window, std::string disk, std::string mode);

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void start();

	std::string mDisk;
	std::string mMode;

	MenuComponent mMenu;
	std::shared_ptr<OptionListComponent<std::string>> moptionsTarget;
	std::shared_ptr<OptionListComponent<std::string>> moptionsEfi;
	std::shared_ptr<SwitchComponent> moptionsValidation;
};
