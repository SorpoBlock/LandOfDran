#pragma once

#include "../LandOfDran.h"

#include "../External/Imgui/imgui.h"
#include "../External/Imgui/imgui_impl_sdl2.h"
#include "../External/Imgui/imgui_impl_opengl3.h"
#include "../External/Imgui//imgui_stdlib.h"

class UserInterface;

/*
	A Imgui window of some sort
	Every window with Imgui should be based around a class that inherits this one
*/
class Window
{
	protected:

	friend class UserInterface;

	//Displayed in titlebar, can be changed
	std::string name = "";

	//Child class should set this to true at the end of init()
	bool initalized = false;

	//Called by UserInterface on start up
	virtual void init() = 0;
	//Called by UserInterface every frame
	virtual void render(ImGuiIO* io) = 0;
	//Is window currently open and being rendered
	bool opened = false;
	 
	//The instance that owns this Window
	UserInterface* userInterface = nullptr;

	explicit Window() = default;

	public:

	virtual ~Window() = default;

	//Opens the window if it was closed
	void open();
};

/*
	Holds everything we need to render user interfaces
	Each (major) window gets its own .h and .cpp file that includes this file
*/
class UserInterface
{
	//Every UI element in the program
	std::vector<std::shared_ptr<Window>> windows;

	ImGuiIO* io = nullptr;

	//Just increases font size really, but Dear Imgui will increase component sizes with text in them
	float uiScaling = 1.0;

	//Opacity/Transparency for all windows unless they stack or something
	float globalInterfaceTransparency = 1.0;
	
	public:

	void updateSettings(std::shared_ptr<SettingManager> settings);

	bool wantsSuppression() const;

	//How many windows are currently open
	int getOpenWindowCount() const;

	/*
		Call in your SDL_PollEvent loop
		Returns if InputMap should be suppressed
	*/
	void handleInput(SDL_Event& e) const;

	//Creates a window of a given derived type and returns a shared pointer to it
	template <typename T,typename ... Args>
	std::shared_ptr<T> createWindow(Args... args)
	{
		std::shared_ptr<T> window(new T(args...));
		window->userInterface = this;
		windows.push_back(window);
		return window;
	}

	//Call every frame
	void render();

	/*
		Call after all windows have been added :
		This is mostly in case a window needs to grab a reference to another window to function
	*/
	void initAll();

	//Returns nullptr if nothing by that name
	std::shared_ptr<Window> getWindowByName(const std::string &name);

	//If mouselock should be forced on
	bool shouldUnlockMouse() const;

	//Trigged if you hit escape, ideally, do it again and again until all windows are closed
	void closeOneWindow();

	/*
		Initialize Imgui
		Must be called after a render context is created
	*/
	UserInterface();

	//Shuts down Imgui but not before deleting every Window 
	~UserInterface();
};
