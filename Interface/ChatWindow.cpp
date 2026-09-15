#include "ChatWindow.h"

void ChatWindow::addMessage(const std::string &message)
{
	messages.push_back(message);
}

void ChatWindow::startTyping()
{
	if (!opened || typing)
		return;

	typing = true;
	focusRequested = true;
	focusFramesLeft = focusFrames;
	releaseFocus = false;
}

void ChatWindow::stopTyping(bool cancel)
{
	if (cancel)
	{
		messageBuffer[0] = 0;
		clearAfterRelease = true;
	}

	if (!typing)
		return;

	typing = false;
	focusRequested = false;
	focusFramesLeft = 0;
	releaseFocus = true;
}

void ChatWindow::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if (releaseFocus)
	{
		//Also ends the message bar being active, so nothing keeps the game's keys suppressed
		ImGui::SetWindowFocus(nullptr);
		releaseFocus = false;
	}

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
	if (focusRequested)
		ImGui::SetNextWindowCollapsed(false);

	//No close button, and it never takes the keyboard from being clicked or appearing, only from startTyping
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	if (!ImGui::Begin("Chat Window", nullptr, flags))
	{
		//Collapsed, which ends typing
		if (typing)
		{
			stopTyping(false);
			ImGui::SetWindowFocus(nullptr);
			releaseFocus = false;
		}
		ImGui::End();
		return;
	}

	//Messages fill whatever the scroll lock and message bar rows leave
	ImGui::BeginChild("chatScroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2));
	ImGui::PushTextWrapPos(0.0f);

	//Unformatted, since a message is whatever someone typed, percent signs included
	for (const std::string& message : messages)
		ImGui::TextUnformatted(message.c_str());

	if(scrollLock)
		ImGui::SetScrollY(ImGui::GetScrollMaxY());

	ImGui::PopTextWrapPos();
	ImGui::EndChild();

	ImGui::Checkbox("Scroll Lock", &scrollLock);

	if (focusRequested)
	{
		//The chat key's own letter is waiting to be typed this frame
		io->InputQueueCharacters.resize(0);
		ImGui::SetKeyboardFocusHere();
		focusRequested = false;
	}

	std::string hint = typing ? "Enter to send, Escape to cancel" : "Press " + chatKeyName + " to chat";
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::BeginDisabled(!typing);
	bool submitted = ImGui::InputTextWithHint("##Message", hint.c_str(), messageBuffer, sizeof(messageBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
	bool active = ImGui::IsItemActive();
	ImGui::EndDisabled();

	if (clearAfterRelease && !typing)
	{
		messageBuffer[0] = 0;
		clearAfterRelease = false;
	}

	if (submitted)
	{
		if (strlen(messageBuffer) > 0)
		{
			chatMessage = messageBuffer;
			chatMessageWaiting = true;
			messageBuffer[0] = 0;
		}
		stopTyping(false);
	}
	else if (typing)
	{
		//The keyboard is handed over a frame or two after it's asked for, and once it's taken, losing it means something else was clicked
		if (active)
			focusFramesLeft = 0;
		else if (focusFramesLeft > 0)
			focusFramesLeft--;
		else
			stopTyping(false);
	}

	ImGui::End();
}

ChatWindow::ChatWindow()
{
	name = "Chat Window";
	hudWindow = true;
}

ChatWindow::~ChatWindow()
{

}

void ChatWindow::init()
{

}

//The chat key starts typing, unless something else already has the keyboard
void ChatWindow::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{
	SDL_Scancode chatKey = input->getKeyBind(OpenChatWindow);
	const char* keyName = SDL_GetScancodeName(chatKey);
	chatKeyName = keyName && keyName[0] ? keyName : "the chat key";

	if (e.type != SDL_KEYDOWN || e.key.repeat || e.key.keysym.scancode != chatKey)
		return;

	if (opened && !typing && !ImGui::GetIO().WantCaptureKeyboard)
		startTyping();
}
