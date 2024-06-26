#include "ChatWindow.h"

void ChatWindow::addMessage(const std::string &message)
{
	messages.push_back(message);
}

void ChatWindow::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if(!ImGui::Begin("Chat Window", &opened))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginGroup();
	float windowWidth = ImGui::GetContentRegionAvail().x;
	ImGui::BeginChild("chatScroll", ImVec2(windowWidth, 200));
	ImGui::PushTextWrapPos(0.0f);

	auto iter = messages.begin();
	while (iter != messages.end())
	{
		ImGui::Text(iter->c_str());
		iter++;
	}

	if(scrollLock)
		ImGui::SetScrollY(ImGui::GetScrollMaxY());

	ImGui::PopTextWrapPos();
	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::Checkbox("Scroll Lock", &scrollLock);

	bool chatSubmitted = false;

	if (ImGui::InputText("Message", messageBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue))
		chatSubmitted = true; //Pressed return while typing

	if(ImGui::Button("Send"))
		chatSubmitted = true; //Hit send button

	if(chatSubmitted)
	{
		if(strlen(messageBuffer) > 0)
		{
			chatMessage = messageBuffer;
			chatMessageWaiting = true;
			messageBuffer[0] = 0;
		}
	}

	ImGui::End();
}

ChatWindow::ChatWindow()
{
	name = "Chat Window";
}

ChatWindow::~ChatWindow()
{

}

void ChatWindow::init()
{

}

//Handle return key to send chat message
void ChatWindow::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{

}
