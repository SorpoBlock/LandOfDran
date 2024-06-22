#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"

enum EscapeButtonPressed
{
	None = 0,
	LeaveServer = 1,
	LeaveGame = 2
};

class EscapeMenu : public Window
{
	friend class Window;
	friend class UserInterface;

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	EscapeButtonPressed lastButtonPress = EscapeButtonPressed::None;

	EscapeMenu();

public:

	//Get last button press type since the last time this was called
	EscapeButtonPressed getLastButtonPress();

	~EscapeMenu();
};
