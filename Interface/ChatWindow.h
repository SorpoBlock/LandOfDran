#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"

class ChatWindow : public Window
{
	std::vector<std::string> messages;

	bool scrollLock = true;

	char messageBuffer[256] = { 0 };

public:
	void addMessage(const std::string &message);

	ChatWindow();
	~ChatWindow();

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;
};

