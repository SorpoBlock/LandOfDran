#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"
#include "../Utility/SettingManager.h"

class SettingsMenu : public Window
{
	friend class Window;

	std::shared_ptr<SettingManager> settings;

	void render(ImGuiIO* io) override;
	void init() override;

	public:
	
	SettingsMenu(std::shared_ptr< SettingManager> _settings);
	~SettingsMenu();
};
