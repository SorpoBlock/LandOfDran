#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"
#include "../Utility/SettingManager.h"
#include "../Interface/InputMap.h"

class SettingsMenu : public Window
{
	friend class Window;

	std::shared_ptr<SettingManager> settings;
	std::shared_ptr<InputMap> inputMap;

	void render(ImGuiIO* io) override;
	void init() override;

	bool settingsUpdated = false;

	//Will be NoCommand is we're not binding a key
	InputCommand currentlyBindingFor = InputCommand::NoCommand;

	public:

	//Put this in your main SDL_PollEvent loop, won't do anything unless you're binding a key tho
	void processKeyBind(SDL_Event& e);

	//Did someone hit the save button since the last call to this function
	bool pollForChanges();
	
	SettingsMenu(std::shared_ptr< SettingManager> _settings,
		std::shared_ptr<InputMap> _inputMap);
	~SettingsMenu();
};
