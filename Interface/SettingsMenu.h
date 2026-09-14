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
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	bool settingsUpdated = false;

	//Will be NoCommand is we're not binding a key
	InputCommand currentlyBindingFor = InputCommand::NoCommand;

	//Lower case paths of string settings shown as a drop down, see setStringChoices, and what each gave when its drop down was last opened
	std::map<std::string, std::function<std::vector<std::string>()>> stringChoiceSources;
	std::map<std::string, std::vector<std::string>> stringChoices;

	SettingsMenu(std::shared_ptr< SettingManager> _settings,
		std::shared_ptr<InputMap> _inputMap);

	public:

	/*
		Shows the string setting at path (like audio/microphone) as a drop down of what choices returns instead of a text box
		choices is asked again each time the drop down opens, since things like microphones can change while the game runs
		The chosen text is what's saved
	*/
	void setStringChoices(const std::string& path, std::function<std::vector<std::string>()> choices);

	//Did someone hit the save button since the last call to this function
	bool pollForChanges();
	
	~SettingsMenu();
};
