#include "guis/GuiNetworkSources.h"

#include "ApiSystem.h"
#include "GuiLoading.h"
#include "LocaleES.h"
#include "Settings.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "utils/StringUtil.h"

#include <functional>
#include <string>
#include <vector>

namespace
{
	using TextCallback = std::function<void(const std::string&)>;

	void askText(Window* window, const std::string& title, const TextCallback& onOk)
	{
		if (Settings::getInstance()->getBool("UseOSK"))
			window->pushGui(new GuiTextEditPopupKeyboard(window, title, "", onOk, false));
		else
			window->pushGui(new GuiTextEditPopup(window, title, "", onOk, false));
	}

	void askCredentials(Window* window, const std::string& host, const std::function<void(const std::string&, const std::string&)>& onOk)
	{
		askText(window, _("USERNAME FOR") + " " + host, [window, onOk](const std::string& user)
		{
			askText(window, _("PASSWORD"), [onOk, user](const std::string& password) { onOk(user, password); });
		});
	}

	bool isSmbAddress(const std::string& address)
	{
		for (const char* prefix : { "//", "\\\\", "smb://", "cifs://" })
			if (Utils::String::startsWith(address, prefix))
				return true;
		return false;
	}

	std::string shareHost(const std::string& uri)
	{
		if (Utils::String::startsWith(uri, "nfs:"))
		{
			std::string rest = uri.substr(4);
			return rest.substr(0, rest.find(':')) + " (NFS)";
		}
		std::string rest = uri.substr(7);
		return rest.substr(0, rest.find('/')) + " (SMB)";
	}

	std::string shareLabel(const std::string& uri, const std::string& name)
	{
		return name + "  [" + shareHost(uri) + "]";
	}

	void addSource(Window* window, GuiSettings* menu, const std::string& address, const std::string& user = "", const std::string& password = "")
	{
		window->pushGui(new GuiLoading<bool>(window, _("ADDING NETWORK SHARE..."),
			[address, user, password](auto)
			{
				return ApiSystem::getInstance()->storageManagerAddNetwork(address, user, password);
			},
			[window, menu](bool added)
			{
				menu->close();
				GuiNetworkSources::open(window);
				window->pushGui(new GuiMsgBox(window, added
					? _("NETWORK SHARE ADDED. ITS GAMES APPEAR ONCE IT IS MOUNTED.")
					: _("THIS ADDRESS WAS NOT RECOGNISED. USE //HOST/SHARE FOR SMB OR HOST:/EXPORT FOR NFS.")));
			}));
	}

	void confirmShare(Window* window, GuiSettings* menu, const std::vector<GuiSettings*>& pickers,
		const std::string& uri, const std::string& name, const std::string& user = "", const std::string& password = "")
	{
		std::string message = _("NETWORK SHARE") + "\n\n" +
			_("SHARE") + ": " + name + "\n" +
			_("HOST") + ": " + shareHost(uri) + "\n\n" +
			_("Merge games from this share now?") + "\n" +
			_("(This will also apply on future boots)");
		window->pushGui(new GuiMsgBox(window, message,
			_("YES, MERGE SHARE"), [window, menu, pickers, uri, user, password]
			{
				for (auto* picker : pickers)
					picker->close();
				addSource(window, menu, uri, user, password);
			},
			_("NO"), nullptr));
	}

	void signIn(Window* window, GuiSettings* menu, GuiSettings* found, const std::string& host)
	{
		askCredentials(window, host, [window, menu, found, host](const std::string& user, const std::string& password)
		{
			window->pushGui(new GuiLoading<std::vector<std::string>>(window, _("SIGNING IN..."),
				[host, user, password](auto)
				{
					return ApiSystem::getInstance()->listNetworkShares(host, user, password);
				},
				[window, menu, found, host, user, password](std::vector<std::string> lines)
				{
					if (lines.empty())
					{
						window->pushGui(new GuiMsgBox(window, _("COULD NOT SIGN IN, OR THIS COMPUTER SHARES NO FOLDERS.")));
						return;
					}
					auto* shares = new GuiSettings(window, host.c_str());
					for (const auto& line : lines)
					{
						auto fields = Utils::String::split(line, '\t');
						if (fields.size() < 2)
							continue;
						std::string uri = fields[0];
						std::string name = fields[1];
						shares->addEntry(shareLabel(uri, name), false, [window, menu, found, shares, uri, name, user, password]
						{
							confirmShare(window, menu, { shares, found }, uri, name, user, password);
						});
					}
					window->pushGui(shares);
				}));
		});
	}

	void scan(Window* window, GuiSettings* menu)
	{
		window->pushGui(new GuiLoading<std::vector<std::string>>(window, _("SEARCHING THE NETWORK..."),
			[](auto)
			{
				return ApiSystem::getInstance()->scanNetworkShares();
			},
			[window, menu](std::vector<std::string> lines)
			{
				if (lines.empty())
				{
					window->pushGui(new GuiMsgBox(window, _("NO SHARED FOLDERS FOUND. IF YOURS IS MISSING, USE ADD BY ADDRESS.")));
					return;
				}
				auto* found = new GuiSettings(window, _("SHARES FOUND").c_str());
				for (const auto& line : lines)
				{
					auto fields = Utils::String::split(line, '\t');
					if (fields.size() < 4)
						continue;
					std::string kind = fields[1];
					std::string uri = fields[2];
					std::string name = fields[3];
					if (kind == "signin")
					{
						found->addEntry(name + "  [" + _("SIGN IN") + "]", true, [window, menu, found, name]
						{
							signIn(window, menu, found, name);
						});
						continue;
					}
					found->addEntry(shareLabel(uri, name), false, [window, menu, found, uri, name]
					{
						confirmShare(window, menu, { found }, uri, name);
					});
				}
				window->pushGui(found);
			}));
	}

	void addByAddress(Window* window, GuiSettings* menu)
	{
		askText(window, _("SHARE ADDRESS: //HOST/SHARE OR HOST:/EXPORT"), [window, menu](const std::string& entered)
		{
			std::string address = Utils::String::trim(entered);
			if (address.empty())
				return;
			if (!isSmbAddress(address))
			{
				addSource(window, menu, address);
				return;
			}
			window->pushGui(new GuiMsgBox(window, _("DOES THIS SHARE NEED A USERNAME AND PASSWORD?"),
				_("YES"), [window, menu, address]
				{
					askCredentials(window, address, [window, menu, address](const std::string& user, const std::string& password)
					{
						addSource(window, menu, address, user, password);
					});
				},
				_("NO"), [window, menu, address] { addSource(window, menu, address); }));
		});
	}

	void addConfiguredSource(Window* window, GuiSettings* menu, const std::string& line)
	{
		auto fields = Utils::String::split(line, '|');
		if (fields.size() < 3)
			return;
		std::string source = fields[0];
		std::string status = fields[2];
		std::string label = source.substr(0, source.find(' '));
		menu->addEntry(label + "  [" + status + "]", true, [window, menu, source, label, status]
		{
			auto* actions = new GuiSettings(window, label.c_str());
			if (status != "mounted")
			{
				actions->addEntry(_("MOUNT"), false, [window, menu, actions]
				{
					ApiSystem::getInstance()->storageManagerMountNetwork();
					GuiSettings* sources = menu;
					auto reopen = [window] { GuiNetworkSources::open(window); };
					actions->close();
					sources->close();
					reopen();
				});
			}
			actions->addEntry(_("REMOVE"), false, [window, menu, actions, source, label]
			{
				window->pushGui(new GuiMsgBox(window, _("REMOVE THIS NETWORK SOURCE?") + "\n" + label,
					_("REMOVE"), [window, menu, actions, source]
					{
						ApiSystem::getInstance()->storageManagerRemoveNetwork(source);
						actions->close();
						menu->close();
						GuiNetworkSources::open(window);
					},
					_("CANCEL"), nullptr));
			});
			window->pushGui(actions);
		});
	}
}

void GuiNetworkSources::open(Window* window)
{
	auto* menu = new GuiSettings(window, _("NETWORK SOURCES").c_str());
	menu->addEntry(_("SCAN FOR SHARES"), true, [window, menu] { scan(window, menu); });
	menu->addEntry(_("ADD BY ADDRESS"), true, [window, menu] { addByAddress(window, menu); });

	auto lines = ApiSystem::getInstance()->storageManagerListNetwork();
	if (!lines.empty())
	{
		menu->addGroup(_("ADDED SHARES"));
		for (const auto& line : lines)
			addConfiguredSource(window, menu, line);
	}
	window->pushGui(menu);
}
