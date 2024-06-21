#include "UserInterface.h"

void UserInterface::updateSettings(std::shared_ptr<SettingManager> settings)
{
	ImGuiStyle& style = ImGui::GetStyle();

	PreferencePair const * pref = settings->getPreference("gui/textcolor");
	if (pref)
		style.Colors[ImGuiCol_Text] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);

	pref = settings->getPreference("gui/windowcolor");
	if (pref)
		style.Colors[ImGuiCol_WindowBg] = ImVec4(pref->color[0],pref->color[1],pref->color[2],pref->color[3]);

	pref = settings->getPreference("gui/framecolor");
	if (pref)
	{
		style.Colors[ImGuiCol_FrameBg] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_Button] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_Tab] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_Header] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
	}

	pref = settings->getPreference("gui/framehovercolor");
	if (pref)
	{
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
	}

	pref = settings->getPreference("gui/highlight");
	if (pref)
	{
		style.Colors[ImGuiCol_CheckMark] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
	}

	pref = settings->getPreference("gui/frameclickcolor");
	if (pref)
	{
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_TabActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);
	}

	pref = settings->getPreference("gui/titlecolor");
	if (pref)
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(pref->color[0], pref->color[1], pref->color[2], pref->color[3]);

	float rounding = settings->getFloat("gui/rounding");
	style.WindowRounding = rounding;
	style.ChildRounding = rounding;
	style.FrameRounding = rounding;
	style.PopupRounding = rounding;
	style.GrabRounding = rounding;

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
bool UserInterface::shouldUnlockMouse() const
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

bool UserInterface::handleInput(SDL_Event& e,std::shared_ptr<InputMap> input)
{
	ImGui_ImplSDL2_ProcessEvent(&e);

	for (unsigned int a = 0; a < windows.size(); a++)
		windows[a]->handleInput(e, input);

	//Don't open other windows while typing in a window already
	if (io->WantCaptureKeyboard)
		return false;

	if (e.type != SDL_KEYDOWN)
		return false;

	if (e.key.keysym.scancode == input->getKeyBind(OpenDebugWindow))
	{
		auto tmp = getWindowByName("Debug Menu");
		if (tmp)
			tmp->open();
		return true;
	}

	if (e.key.keysym.scancode == input->getKeyBind(OpenOptionsMenu))
	{
		auto tmp = getWindowByName("Settings Menu");
		if (tmp)
			tmp->open();
		return true;
	}

	return false;
}

std::shared_ptr<Window> UserInterface::getWindowByName(const std::string &name)
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
