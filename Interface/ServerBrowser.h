#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"

class ServerBrowser : public Window
{
	friend class Window;
	friend class UserInterface;

	char serverAddressBuffer[256];
	int serverPort = DEFAULT_PORT;
	//This is set when user clicks join button and reset when you call getServerData
	bool serverPicked = false;

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	ServerBrowser();

public:

	//Has the user clicked the join button recently with valid parameters
	bool serverPickReady() const { return serverPicked; }

	//Get the parameters after serverPicked returns true
	void getServerData(std::string &ip,int &port);

	~ServerBrowser();
};
