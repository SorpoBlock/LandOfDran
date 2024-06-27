#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"

class ChatWindow : public Window
{
	std::vector<std::string> messages;

	bool scrollLock = true;

	char messageBuffer[256] = { 0 };
	std::string chatMessage = "";
	bool chatMessageWaiting = false;

public:
	void addMessage(const std::string &message);

	ChatWindow();
	~ChatWindow();

	bool hasChatMessage() const { return chatMessageWaiting; }

	//Resets hasChatMessage
	std::string getChatMessage() { chatMessageWaiting = false; return chatMessage; }

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;
};

