#pragma once

#include "../LandOfDran.h"

#include "../Networking/Server.h"
#include "ServerProgramData.h"
#include "../LuaFunctions/OtherFunctions.h"

/*
	This is the big bad class that allows us to separate our server hosting loop from
	our client playing loop along with all the variables and structures specific to it
*/
class LoopServer
{
	//Stuff we need to *play* the game, as opposed to host it, except our net interface itself
	ServerProgramData pd;
	//Network connection manager, its methods take ClientProgramData as a parameter, so it's separate
	Server* server = nullptr;

	bool valid = false;

	//Testing only:
	float spin = 0;

public:

	//Constructor have any issues?
	bool isValid() const { return valid; }

	//Lot less in this so far given no input or rendering on the server...
	void run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);

	//Called only when the program starts up
	LoopServer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);
	//Called only when the program finally shuts down
	~LoopServer();
};
