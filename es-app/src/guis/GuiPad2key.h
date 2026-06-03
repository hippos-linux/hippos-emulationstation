#pragma once
#ifndef ES_APP_GUIS_GUI_PAD2KEY_H
#define ES_APP_GUIS_GUI_PAD2KEY_H

#include "GuiSettings.h"

struct Pad2key;

class GuiPad2key : public GuiSettings
{
public:
	GuiPad2key(Window* window, Pad2key ktp);
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:	
	void loadActivePage(const std::string& device_path);
	void loadPlayerPage(int n, const std::string& device_path);
	void loadHotkeysPage(const std::string& device_path);

  	Pad2keyDevice* getJoystickDevice(int n);
  	Pad2keyDevice* getHotkeyDevice();
  	int countJoystickDevice(const std::string& type);

  	std::string getNameForInput(Pad2keyDevice* device, std::string value);
  	void declareEvKey(Window* window, const std::string& device_path, const std::function<void(std::string)>& func);

  	void setDeviceValue(Pad2keyDevice* ktp, std::string key, std::string value, Window* window, const std::string& device_path);

	Pad2key m_ktp;
	std::vector<Pad2keyDevice> m_devices;
	bool m_need_save;
};

#endif // ES_APP_GUIS_GUI_PAD2KEY_H
