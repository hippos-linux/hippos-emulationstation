#pragma once

#include "GuiComponent.h"
#include "components/BusyComponent.h"
#include <thread>
#include <string>

class GuiClone : public GuiComponent
{
public:
	GuiClone(Window* window, std::string sourceDisk, std::string targetDisk);
	~GuiClone();

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void threadClone();
	void onCloneOk();
	void onCloneError(std::pair<std::string, int> result);

	BusyComponent mBusyAnim;
	bool mLoading;
	int mState;
	std::pair<std::string, int> mResult;
	std::thread* mHandle;
	std::string mSourceDisk;
	std::string mTargetDisk;
};
