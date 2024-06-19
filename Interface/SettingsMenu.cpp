#include "SettingsMenu.h"

void SettingsMenu::init()
{
	initalized = true;
}

bool SettingsMenu::pollForChanges()
{
	bool ret = settingsUpdated;
	settingsUpdated = false;
	return ret;
}
void SettingsMenu::processKeyBind(SDL_Event& e)
{
	if(currentlyBindingFor == InputCommand::NoCommand)
		return;

	if (e.type != SDL_KEYDOWN)
		return;

	inputMap->bindKey(currentlyBindingFor, e.key.keysym.scancode);
	currentlyBindingFor = InputCommand::NoCommand;
}

void SettingsMenu::render(ImGuiIO* io)
{
	if (currentlyBindingFor != InputCommand::NoCommand)
	{
		ImGui::OpenPopup("Bind Key");
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Bind Key", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::string name = "Press any key to bind to " + GetInputCommandString(currentlyBindingFor);
			ImGui::Text(name.c_str());
			if (ImGui::Button("Cancel"))
				currentlyBindingFor = InputCommand::NoCommand;
			ImGui::EndPopup();
		}
	}

	if (!opened)
		return;

	if (!ImGui::Begin("Preferences and Settings", &opened))
	{
		ImGui::End();
		return;
	}

	//Shared across all text fields, set by the function that renders them when rendered
	char textInput[256];

	//Sets internal iterator to zero for a non-recursive search of the preference node tree
	settings->startPreferenceBindingSearch();

	PreferencePair* ptr = nullptr;
	std::string path = "";
	std::string lastPath = "";
	bool lastTabCreated = false;

	ImGui::BeginTabBar("tabsSettings");

	//For each preference
	while (ptr = settings->nextPreferenceBinding(path))
	{
		//Skip keybinds, we will handle these separatly, getting them right from InputMap
		if (path == "keybinds")
			continue;

		//This whole block just figures out when we want a tab for a new category of controls
		//I.e. 'graphics' or 'audio' if you rearrange the settings in the text file this might break?
		if (lastPath != path)
		{
			if (lastPath != "")
			{
				if (lastTabCreated)
				{
					ImGui::EndTabItem();
					lastTabCreated = false;
				}
			}
			lastTabCreated = ImGui::BeginTabItem(path.c_str());
			if (!lastTabCreated)
				continue;

			lastPath = path;
		}

		//What kind of input component should we make for the given preference
		switch (ptr->type)
		{
		case PreferenceBoolean:
		{
			ImGui::Checkbox(ptr->description.c_str(), &ptr->valueBool);

			break;
		}
		case PreferenceFloat:
		{
			ImGui::SliderFloat(ptr->description.c_str(), &ptr->valueFloat, ptr->minValue, ptr->maxValue);

			break;
		}
		case PreferenceString:
		{
			memcpy(textInput, ptr->value.c_str(), ptr->value.length());
			textInput[ptr->value.length()] = 0;
			ImGui::InputText(ptr->description.c_str(), textInput, 255);
			ptr->value = std::string(textInput);

			break;
		}
		case PreferenceInteger:
		{
			if (ptr->dropDownNames.size() > 0)
			{
				//Add text in the middle of the drop down clarifying what that enum value represents
				//I.e. 'high' or 'medium' or 'low'
				ImGui::SliderInt(ptr->description.c_str(), &ptr->valueInt, ptr->minValue, ptr->maxValue - 1, ptr->dropDownNames[ptr->valueInt].c_str());
			}
			else
				ImGui::SliderInt(ptr->description.c_str(), &ptr->valueInt, ptr->minValue, ptr->maxValue);

			break;
		}
		}
	}

	if (lastTabCreated)
		ImGui::EndTabItem();

	if (ImGui::BeginTabItem("Keybinds"))
	{
		float width = ImGui::GetContentRegionAvail().x;
		float height = ImGui::GetContentRegionAvail().y;

		int flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
		if (ImGui::BeginTable("keybinds",2,flags,ImVec2(width * 0.95,height * 0.90)))
		{
			ImGui::TableSetupColumn("Command");
			ImGui::TableSetupColumn("Bound Key");
			ImGui::TableHeadersRow();

			//NoCommand is index 0
			for (int a = 1; a < InputCommand::EndOfCommands; a++)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				//What command are we setting
				ImGui::Text(GetInputCommandString((InputCommand)a).c_str());
				ImGui::TableNextColumn();
				//Button you click to bind a key to that
				if (ImGui::Button(SDL_GetScancodeName(inputMap->getKeyBind((InputCommand)a)), ImVec2(width * 0.95 * 0.5, 20)))
					currentlyBindingFor = (InputCommand)a;
			}

			ImGui::EndTable();
		}
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();

	ImGui::NewLine();
	if (ImGui::Button("Save Changes"))
	{
		info("Saving settings...");
		inputMap->setPreferences(settings);
		settings->exportToFile("Config/settings.txt");
		//Main loop will have to poll settings to actually change how the game runs
		settingsUpdated = true;
	}

	ImGui::End();
}

SettingsMenu::SettingsMenu(
	std::shared_ptr< SettingManager> _settings,
	std::shared_ptr<InputMap> _inputMap) 
	: settings(_settings),
	inputMap(_inputMap)
{

}

SettingsMenu::~SettingsMenu()
{
	settings.reset();
}
