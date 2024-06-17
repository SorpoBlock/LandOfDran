#include "SettingsMenu.h"

static int MyResizeCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		ImVector<char>* my_str = (ImVector<char>*)data->UserData;
		IM_ASSERT(my_str->begin() == data->Buf);
		my_str->resize(data->BufSize);
		data->Buf = my_str->begin();
	}
	return 0;
}

void SettingsMenu::init()
{
	initalized = true;
}

void SettingsMenu::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if (!ImGui::Begin("Preferences and Settings", &opened))
	{
		ImGui::End();
		return;
	}

	char* textInput = new char[256];

	settings->startPreferenceBindingSearch();

	PreferencePair* ptr = nullptr;
	std::string path = "";
	std::string lastPath = "";
	bool lastTabCreated = false;

	ImGui::BeginTabBar("tabsSettings");

	while (ptr = settings->nextPreferenceBinding(path))
	{
		if (path == "keybinds")
			continue;

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

	delete[] textInput;
	ImGui::EndTabBar();

	ImGui::NewLine();
	ImGui::Button("Save Changes");

	ImGui::End();
}

SettingsMenu::SettingsMenu(std::shared_ptr< SettingManager> _settings) : settings(_settings)
{

}

SettingsMenu::~SettingsMenu()
{
	settings.reset();
}
