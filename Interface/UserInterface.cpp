#include "UserInterface.h"

void UserInterface::updateSettings(std::shared_ptr<SettingManager> settings)
{
	globalInterfaceTransparency = settings->getFloat("gui/opacity");

	//Refer to DefaultPreferences.cpp - Small, Normal, Large, Largest
	int sizeEnum = settings->getInt("gui/scaling");
	switch (sizeEnum)
	{
		case 0: 
			uiScaling = 0.5;
			break;
		case 1:
			uiScaling = 1.0;
			break;
		case 2:
			uiScaling = 1.5;
			break;
		case 3:
			uiScaling = 2.0;
			break;
	}
}

bool UserInterface::wantsSuppression() const
{
	return io->WantCaptureKeyboard;
}

//If mouselock should be forced on
bool UserInterface::shouldUnlockMouse()
{
	return io->WantCaptureMouse;
}

int UserInterface::getOpenWindowCount() const
{
	int ret = 0;
	for (unsigned int a = 0; a < windows.size(); a++)
		if (windows[a]->opened)
			ret++;
	return ret;
}

//Trigged if you hit escape, ideally, do it again and again until all windows are closed
void UserInterface::closeOneWindow()
{
	for (unsigned int a = 0; a < windows.size(); a++)
	{
		if (windows[a]->opened)
		{
			windows[a]->opened = false;
			return;
		}
	}
}

void Window::open()
{
	//TODO: Make new windows appear in upper left corner and slighty offset depending on how many are open
	opened = true;
}

void UserInterface::addWindow(std::shared_ptr<Window> window)
{
	window->userInterface = this;
	windows.push_back(window);
}

void UserInterface::handleInput(SDL_Event& e)
{
	ImGui_ImplSDL2_ProcessEvent(&e);
}

std::shared_ptr<Window> UserInterface::getWindowByName(std::string name)
{
	for (unsigned int a = 0; a < windows.size(); a++)
		if (windows[a]->name == name)
			return windows[a];
	return nullptr;
}

void UserInterface::initAll()
{
	for (unsigned int a = 0; a < windows.size(); a++)
		windows[a]->init();
}

void UserInterface::render()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	for (unsigned int a = 0; a < windows.size(); a++)
	{
		ImGui::SetNextWindowBgAlpha(globalInterfaceTransparency);
		windows[a]->render(io);
	}
	io->FontGlobalScale = uiScaling;
	ImGui::Render();
	ImDrawData* data = ImGui::GetDrawData();
	ImGui_ImplOpenGL3_RenderDrawData(data);
}

UserInterface::UserInterface()
{
	io = &ImGui::GetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	//It's so weird that ImGui handles it this way lol
	const char* glsl_version = "#version 330";
	ImGui_ImplOpenGL3_Init(glsl_version);
}

UserInterface::~UserInterface()
{
	for (unsigned int a = 0; a < windows.size(); a++)
		windows[a].reset();
	windows.clear();
}
