#include "guis/GuiHandheldSettings.h"

#include "ApiSystem.h"
#include "LocaleES.h"
#include "Window.h"
#include "components/OptionListComponent.h"
#include "guis/GuiSettings.h"
#include "utils/StringUtil.h"

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	struct HandheldSetting
	{
		std::string type;
		std::string key;
		std::string title;
		std::string value;
		std::string options;
		std::string hint;
	};

	HandheldSetting parseSetting(const std::string& line)
	{
		std::vector<std::string> fields;
		std::stringstream stream(line);
		std::string field;
		while (std::getline(stream, field, '\t'))
			fields.push_back(field);
		fields.resize(6);
		return { fields[0], fields[1], fields[2], fields[3], fields[4], fields[5] };
	}

	std::shared_ptr<OptionListComponent<std::string>> makeChoice(Window* window, const HandheldSetting& setting)
	{
		auto choice = std::make_shared<OptionListComponent<std::string>>(window, setting.title, false);
		for (const auto& option : Utils::String::split(setting.options, '|'))
		{
			auto separator = option.find('=');
			if (separator == std::string::npos)
				continue;
			std::string value = option.substr(0, separator);
			choice->add(option.substr(separator + 1), value, value == setting.value);
		}
		if (!choice->hasSelection())
			choice->selectFirstItem();
		return choice;
	}
}

void GuiHandheldSettings::open(Window* window)
{
	auto s = new GuiSettings(window, _("HANDHELD CONTROLLER SETTINGS"));
	auto changes = std::make_shared<std::map<std::string, std::string>>();

	for (const auto& line : ApiSystem::getInstance()->listHandheldSettings())
	{
		HandheldSetting setting = parseSetting(line);
		if (setting.type == "group")
		{
			s->addGroup(Utils::String::toUpper(setting.title));
		}
		else if (setting.type == "multiple")
		{
			auto choice = makeChoice(window, setting);
			std::string key = setting.key;
			choice->setSelectedChangedCallback([changes, key](const std::string& value) { (*changes)[key] = value; });
			s->addWithDescription(_(setting.title.c_str()), _(setting.hint.c_str()), choice);
		}
	}

	s->addSaveFunc([changes]
	{
		if (!changes->empty())
			ApiSystem::getInstance()->setHandheldSettings(*changes);
	});

	window->pushGui(s);
}
