#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"
#include "../Utility/SettingManager.h"
#include "../Interface/InputMap.h"

class SettingsMenu : public Window
{
	friend class Window;
	friend class UserInterface;

	std::shared_ptr<SettingManager> settings;
	std::shared_ptr<InputMap> inputMap;

	//Separated the code specific to key binds from the rest
	//Just call after last tab before EndTabBar in render
	void renderKeybindsMenu(ImGuiIO* io);

	//Renders more settings like background color on the gui tab of the settings menu
	void renderThemeSettings(ImGuiIO* io);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;

	bool settingsUpdated = false;

	//Will be NoCommand is we're not binding a key
	InputCommand currentlyBindingFor = InputCommand::NoCommand;

	SettingsMenu(std::shared_ptr< SettingManager> _settings,
		std::shared_ptr<InputMap> _inputMap);

	public:

	//Put this in your main SDL_PollEvent loop, won't do anything unless you're binding a key tho
	void processKeyBind(SDL_Event& e);

	//Did someone hit the save button since the last call to this function
	bool pollForChanges();
	
	~SettingsMenu();
};
