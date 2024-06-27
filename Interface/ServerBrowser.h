#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"

class ServerBrowser : public Window
{
	friend class Window;
	friend class UserInterface;

	char serverAddressBuffer[256];
	char userNameBuffer[256];
	int serverPort = DEFAULT_PORT;
	//This is set when user clicks join button and reset when you call getServerData
	bool serverPicked = false;

	//Set with passLoadProgress, from cmdArgs signals typesToLoad
	int desiredTypes = 0;
	//Set with passLoadProgress, from cmdArgs simulation dynamicTypes.size
	int loadedTypes = 0;

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	std::string connectionNote = "";

	ServerBrowser();

public:

	//Pass in last used values into input fields
	void passDefaultSettings(std::string ip, int port, std::string username);

	//Pass progress loading into a server here so the UI can display it
	void passLoadProgress(int _desiredTypes, int _loadedTypes);

	void setConnectionNote(const std::string &message);

	//Has the user clicked the join button recently with valid parameters
	bool serverPickReady() const { return serverPicked; }

	//Get the parameters after serverPicked returns true
	void getServerData(std::string &ip,int &port,std::string &username);

	~ServerBrowser();
};
