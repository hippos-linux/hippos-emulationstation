#include "guis/GuiClone.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "Log.h"
#include "ApiSystem.h"
#include "LocaleES.h"

GuiClone::GuiClone(Window* window, std::string sourceDisk, std::string targetDisk)
	: GuiComponent(window), mBusyAnim(window)
{
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	mLoading = true;
	mState = 1;
	mBusyAnim.setSize(mSize);
	mSourceDisk = sourceDisk;
	mTargetDisk = targetDisk;
}

GuiClone::~GuiClone()
{
}

bool GuiClone::input(InputConfig* config, Input input)
{
	return false;
}

std::vector<HelpPrompt> GuiClone::getHelpPrompts()
{
	return std::vector<HelpPrompt>();
}

void GuiClone::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();
	renderChildren(trans);
	Renderer::setMatrix(trans);
	Renderer::drawRect(0.f, 0.f, mSize.x(), mSize.y(), 0x00000011);
	if (mLoading)
		mBusyAnim.render(trans);
}

void GuiClone::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	mBusyAnim.update(deltaTime);

	Window* window = mWindow;

	if (mState == 1)
	{
		mLoading = true;
		mHandle = new std::thread(&GuiClone::threadClone, this);
		mState = 0;
	}

	if (mState == 2)
	{
		window->pushGui(new GuiMsgBox(window, _("CLONE COMPLETE"), _("OK"),
			[this] { mState = -1; }));
		mState = 0;
	}

	if (mState == 3)
	{
		window->pushGui(new GuiMsgBox(window, mResult.first, _("OK"),
			[this] { mState = -1; }));
		mState = 0;
	}

	if (mState == -1)
		delete this;
}

void GuiClone::threadClone()
{
	std::pair<std::string, int> result = ApiSystem::getInstance()->cloneDisk(&mBusyAnim, mSourceDisk, mTargetDisk);
	if (result.second == 0)
		onCloneOk();
	else
		onCloneError(result);
}

void GuiClone::onCloneOk()
{
	mLoading = false;
	mState = 2;
}

void GuiClone::onCloneError(std::pair<std::string, int> result)
{
	mLoading = false;
	mState = 3;
	mResult.first = _("CLONE FAILED") + std::string(": check the system/logs directory");
}
