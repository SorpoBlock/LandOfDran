#include "ServerBrowser.h"

void ServerBrowser::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if (!ImGui::Begin("Server Browser", &opened))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Naturally, this is just a placeholder.");
	ImGui::InputText("IP", serverAddressBuffer, 256);
	ImGui::InputInt("Port", &serverPort);

	bool inputOkay = true;

	//I assume 7 characters is the absolute min for an ip for example 1.1.1.1
	std::string ip = std::string(serverAddressBuffer);
	if (ip.length() < 7)
		inputOkay = false;
	if (serverPort < 0 || serverPort > 65535)
		inputOkay = false;

	if(!inputOkay)
		ImGui::BeginDisabled();
	if (ImGui::Button("Join Server"))
		serverPicked = true;
	if(!inputOkay)
		ImGui::EndDisabled();

	ImGui::End();
}

void ServerBrowser::getServerData(std::string& ip, int& port)
{
	serverPicked = false;
	ip = std::string(serverAddressBuffer);
	port = serverPort;
	opened = false;
}

void ServerBrowser::init()
{

}

void ServerBrowser::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{

}

ServerBrowser::ServerBrowser()
{
	name = "Server Browser";
	serverAddressBuffer[0] = 0; //Ensure recognition as empty string
}

ServerBrowser::~ServerBrowser()
{

}
