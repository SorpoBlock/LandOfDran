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

	//Displayed in titlebar
	std::string name = "";

	//Child class should set this to true at the end of init()
	bool initalized = false;

	//Called by UserInterface on start up
	virtual void init() = 0;
	//Called by UserInterface every frame
	virtual void render(ImGuiIO* io) = 0;
	//Is window currently open and being rendered
	bool opened = false;

	public:

	//Opens the window if it was closed
	void open();
};

/*
	Holds everything we need to render user interfaces
	Each (major) window gets its own .h and .cpp file that includes this one
*/
class UserInterface
{
	//Every UI element in the program
	std::vector<std::shared_ptr<Window>> windows;

	ImGuiIO* io = nullptr;
	
	public:

	//Opacity/Transparency for all windows unless they stack or something
	float globalInterfaceTransparency = 1.0;

	//How many windows are currently open
	int getOpenWindowCount() const;

	/*
		Call in your SDL_PollEvent loop
		Returns if InputMap should be suppressed
	*/
	bool handleInput(SDL_Event& e);

	//Windows passed here will be destroyed when UserInterface is
	void addWindow(std::shared_ptr<Window> window);

	//Call every frame
	void render();

	//Call after all windows have been added:
	void initAll();

	//If mouselock should be forced on
	bool shouldUnlockMouse();

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
