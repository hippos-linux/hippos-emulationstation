#pragma once

#include "GuiComponent.h"
#include "components/MenuComponent.h"

template<typename T>
class OptionListComponent;

class SwitchComponent;

class GuiCloneStart : public GuiComponent
{
public:
	GuiCloneStart(Window* window);
	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void start();

	MenuComponent mMenu;
	std::shared_ptr<OptionListComponent<std::string>> moptionsSource;
	std::shared_ptr<OptionListComponent<std::string>> moptionsTarget;
	std::shared_ptr<SwitchComponent> moptionsValidation;
};
