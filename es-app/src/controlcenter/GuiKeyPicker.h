#pragma once

#include "GuiComponent.h"
#include "resources/Font.h"

#include <functional>
#include <string>
#include <vector>

class GuiKeyPicker : public GuiComponent
{
public:
	GuiKeyPicker(Window* window, const std::string& title, const std::vector<std::string>& currentKeys,
		const std::function<void(const std::vector<std::string>&)>& onPick);

	bool input(InputConfig* config, Input input) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	bool sortsHelpPrompts() override { return false; }
	HelpStyle getHelpStyle() override;

private:
	struct Key
	{
		std::string label;
		std::string code;
		float x, y, w;
	};

	void loadLayout();
	void measure(float& unit, float& left, float& top) const;
	void moveFocus(float dx, float dy);
	void pick(const std::vector<std::string>& keys);

	std::string mTitle;
	std::vector<std::string> mCurrent;
	std::vector<std::string> mCombination;
	std::function<void(const std::vector<std::string>&)> mOnPick;

	std::vector<Key> mKeys;
	float mUnitsWide;
	float mRowsHigh;
	int mFocus;

	std::shared_ptr<Font> mFontTitle;
	std::shared_ptr<Font> mFontKey;
	unsigned int mColorBg, mColorKey, mColorText, mColorSelector, mColorSelected, mColorCurrent;
};
