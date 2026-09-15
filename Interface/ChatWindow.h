#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"

/*
	Pinned to the top right corner while in game, where it can be resized and collapsed but not moved or closed
	The message bar only takes the keyboard after the chat key (OpenChatWindow, T by default), until Enter sends, Escape cancels, or something else is clicked
*/
class ChatWindow : public Window
{
	std::vector<std::string> messages;

	bool scrollLock = true;

	char messageBuffer[256] = { 0 };
	std::string chatMessage = "";
	bool chatMessageWaiting = false;

	//Typing a message in the message bar
	bool typing = false;

	//The message bar is given the keyboard on the next render, and gets this many frames to take it before typing gives up
	bool focusRequested = false;
	static constexpr int focusFrames = 5;
	int focusFramesLeft = 0;

	//Set by stopTyping, the keyboard is taken back from the message bar on the next render
	bool releaseFocus = false;

	//Set by stopTyping for a cancel, the message bar is emptied after it's drawn, since ImGui writes a message bar's text back into the buffer the frame it loses the keyboard
	bool clearAfterRelease = false;

	//What the chat key is bound to, for the hint in the message bar
	std::string chatKeyName = "T";

public:
	void addMessage(const std::string &message);

	ChatWindow();
	~ChatWindow();

	bool hasChatMessage() const { return chatMessageWaiting; }

	//Resets hasChatMessage
	std::string getChatMessage() { chatMessageWaiting = false; return chatMessage; }

	//Puts the keyboard in the message bar, expanding the window if it's collapsed
	void startTyping();

	//Gives the keyboard back, throwing away what was typed if cancel
	void stopTyping(bool cancel);

	bool isTyping() const { return typing; }

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;
};

