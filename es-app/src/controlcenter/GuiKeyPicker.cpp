#include "controlcenter/GuiKeyPicker.h"

#include "renderers/Renderer.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "InputConfig.h"
#include "LocaleES.h"
#include "Log.h"
#include "ThemeData.h"
#include "Window.h"

#include <rapidjson/document.h>
#include <algorithm>
#include <cmath>
#include <memory>

static const char* KEY_LAYOUT_FILE = "/usr/share/hippos/quickmenu/keys.json";
static const char* NOT_MAPPED = "none";
static const float NAVIGATION_COLUMN = 15.5f;
static const float KEY_GAP_UNITS = 0.08f;
static const float KEY_CORNER_UNITS = 0.15f;
static const unsigned int KEY_ALPHA = 0x18;
static const unsigned int COMBINATION_ALPHA = 0x70;
static const float EDGE_SHARE = 0.03f;
static const float TITLE_LINE_HEIGHT = 1.6f;
static const float HELP_LINE_HEIGHT = 2.2f;

static void drawText(const std::shared_ptr<Font>& font, const std::string& text, float x, float y, unsigned int color, const Transform4x4f& trans)
{
	std::unique_ptr<TextCache> cache(font->buildTextCache(text, x, y, color));
	Renderer::setMatrix(trans);
	font->renderTextCache(cache.get());
}

GuiKeyPicker::GuiKeyPicker(Window* window, const std::string& title, const std::vector<std::string>& currentKeys,
	const std::function<void(const std::vector<std::string>&)>& onPick)
	: GuiComponent(window), mTitle(Utils::String::toUpper(title)), mCurrent(currentKeys), mOnPick(onPick),
	mUnitsWide(1), mRowsHigh(1), mFocus(0)
{
	auto theme = ThemeData::getMenuTheme();
	mFontTitle = theme->Text.font;
	mFontKey = theme->TextSmall.font;
	mColorBg = (theme->Background.color & 0xFFFFFF00) | 0xFF;
	mColorText = theme->Text.color;
	mColorKey = (theme->Text.color & 0xFFFFFF00) | KEY_ALPHA;
	mColorSelector = theme->Text.selectorColor;
	mColorSelected = theme->Text.selectedColor;
	mColorCurrent = (theme->Text.selectorColor & 0xFFFFFF00) | COMBINATION_ALPHA;

	loadLayout();

	auto current = std::find_if(mKeys.cbegin(), mKeys.cend(), [this](const Key& k) { return !mCurrent.empty() && k.code == mCurrent.back(); });
	auto escape = std::find_if(mKeys.cbegin(), mKeys.cend(), [](const Key& k) { return k.code == "KEY_ESC"; });
	mFocus = (int)((current != mKeys.cend() ? current : escape != mKeys.cend() ? escape : mKeys.cbegin()) - mKeys.cbegin());

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
}

void GuiKeyPicker::loadLayout()
{
	rapidjson::Document doc;
	doc.Parse(Utils::FileSystem::readAllText(KEY_LAYOUT_FILE).c_str());
	if (doc.HasParseError() || !doc.IsObject())
	{
		LOG(LogError) << "GuiKeyPicker: unable to read " << KEY_LAYOUT_FILE;
		return;
	}

	auto place = [this, &doc](const char* section, float firstColumn, float firstRow)
	{
		if (!doc.HasMember(section) || !doc[section].IsArray())
			return 0.0f;

		float rows = 0;
		for (auto& row : doc[section].GetArray())
		{
			float x = firstColumn;
			for (auto& key : row.GetArray())
			{
				if (key.Size() < 3 || !key[2].IsNumber())
					continue;

				float width = key[2].GetFloat();
				if (key[1].IsString())
					mKeys.push_back({ key[0].GetString(), key[1].GetString(), x, firstRow + rows, width });
				x += width;
				mUnitsWide = std::max(mUnitsWide, x);
			}
			rows++;
		}
		return rows;
	};

	float mainRows = place("main", 0, 0);
	place("navigation", NAVIGATION_COLUMN, 0);
	float extraRows = place("extra", 0, mainRows + 1);
	mRowsHigh = mainRows + 1 + extraRows;
}

void GuiKeyPicker::moveFocus(float dx, float dy)
{
	if (mKeys.empty())
		return;

	const Key& from = mKeys[mFocus];
	float cx = from.x + from.w / 2, cy = from.y;
	int best = -1;
	float bestScore = 0;
	for (int i = 0; i < (int)mKeys.size(); i++)
	{
		const Key& k = mKeys[i];
		float kx = k.x + k.w / 2 - cx, ky = k.y - cy;
		float along = kx * dx + ky * dy;
		if (along <= 0.01f)
			continue;

		float across = std::abs(kx * dy) + std::abs(ky * dx);
		float score = along + 2 * across;
		if (best < 0 || score < bestScore)
		{
			best = i;
			bestScore = score;
		}
	}

	if (best >= 0)
		mFocus = best;
}

void GuiKeyPicker::pick(const std::vector<std::string>& keys)
{
	auto onPick = mOnPick;
	delete this;
	onPick(keys);
}

bool GuiKeyPicker::input(InputConfig* config, Input input)
{
	if (input.value == 0)
		return GuiComponent::input(config, input);

	if (config->isMappedLike("up", input))
		moveFocus(0, -1);
	else if (config->isMappedLike("down", input))
		moveFocus(0, 1);
	else if (config->isMappedLike("left", input))
		moveFocus(-1, 0);
	else if (config->isMappedLike("right", input))
		moveFocus(1, 0);
	else if (config->isMappedTo(BUTTON_OK, input) && !mKeys.empty())
	{
		const std::string& code = mKeys[mFocus].code;
		std::vector<std::string> keys;
		if (code != NOT_MAPPED)
		{
			keys = mCombination;
			if (std::find(keys.cbegin(), keys.cend(), code) == keys.cend())
				keys.push_back(code);
		}
		pick(keys);
		return true;
	}
	else if (config->isMappedTo("x", input) && !mKeys.empty() && mKeys[mFocus].code != NOT_MAPPED)
	{
		const std::string& code = mKeys[mFocus].code;
		auto it = std::find(mCombination.begin(), mCombination.end(), code);
		if (it != mCombination.end())
			mCombination.erase(it);
		else
			mCombination.push_back(code);
	}
	else if (config->isMappedTo("y", input))
	{
		pick({});
		return true;
	}
	else if (config->isMappedTo(BUTTON_BACK, input))
	{
		delete this;
		return true;
	}
	else
		return GuiComponent::input(config, input);

	return true;
}

void GuiKeyPicker::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), mColorBg);

	float unit, left, top;
	measure(unit, left, top);
	float gap = KEY_GAP_UNITS * unit;
	float titleH = mFontTitle->getHeight(TITLE_LINE_HEIGHT);

	drawText(mFontTitle, mTitle, left, top - titleH + (titleH - mFontTitle->getHeight()) / 2, mColorSelector, trans);

	for (int i = 0; i < (int)mKeys.size(); i++)
	{
		const Key& k = mKeys[i];
		bool focused = i == mFocus;
		bool current = std::find(mCurrent.cbegin(), mCurrent.cend(), k.code) != mCurrent.cend();
		bool combined = std::find(mCombination.cbegin(), mCombination.cend(), k.code) != mCombination.cend();

		float x = left + k.x * unit + gap / 2, y = top + k.y * unit + gap / 2;
		float w = k.w * unit - gap, h = unit - gap;
		float corner = KEY_CORNER_UNITS * unit;

		Renderer::setMatrix(trans);
		Renderer::drawRoundRect(x, y, w, h, corner, focused ? mColorSelector : combined ? mColorCurrent : mColorKey);
		if (current && !focused)
		{
			Renderer::drawRoundRect(x, y, w, h, corner, mColorSelector);
			Renderer::drawRoundRect(x + 2, y + 2, w - 4, h - 4, corner, mColorBg);
			Renderer::drawRoundRect(x + 2, y + 2, w - 4, h - 4, corner, mColorKey);
		}

		std::string label = k.label;
		auto size = mFontKey->sizeText(label, 1.0f);
		while (size.x() > w - gap && label.size() > 1)
		{
			label.pop_back();
			size = mFontKey->sizeText(label, 1.0f);
		}
		drawText(mFontKey, label, x + (w - size.x()) / 2, y + (h - size.y()) / 2, focused ? mColorSelected : mColorText, trans);
	}

	if (mWindow->peekGui() == this)
		mWindow->renderHelpPromptsEarly(parentTrans);
}

void GuiKeyPicker::measure(float& unit, float& left, float& top) const
{
	float pad = mSize.y() * EDGE_SHARE;
	float titleH = mFontTitle->getHeight(TITLE_LINE_HEIGHT);
	float helpH = mFontKey->getHeight(HELP_LINE_HEIGHT);
	unit = std::min((mSize.x() - 2 * pad) / mUnitsWide, (mSize.y() - 2 * pad - titleH - helpH) / mRowsHigh);
	left = (mSize.x() - unit * mUnitsWide) / 2;
	top = (mSize.y() - (titleH + unit * mRowsHigh + helpH)) / 2 + titleH;
}

std::vector<HelpPrompt> GuiKeyPicker::getHelpPrompts()
{
	return { HelpPrompt(BUTTON_OK, _("Pick")), HelpPrompt("x", _("Add to combination")), HelpPrompt("y", _("Not mapped")), HelpPrompt(BUTTON_BACK, _("Back")) };
}

HelpStyle GuiKeyPicker::getHelpStyle()
{
	HelpStyle style = GuiComponent::getHelpStyle();
	style.upperCase = false;

	float unit, left, top;
	measure(unit, left, top);
	float helpH = mFontKey->getHeight(HELP_LINE_HEIGHT);
	style.position = Vector2f(mSize.x() / 2, top + unit * mRowsHigh + (helpH - style.font->getHeight()) / 2);
	style.origin = Vector2f(0.5f, 0);
	return style;
}
