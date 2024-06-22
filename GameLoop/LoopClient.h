#pragma once

#include "../LandOfDran.h"

#include "../Networking/Client.h"
#include "ClientProgramData.h"

/*
	This is the big bad class that allows us to separate our client playing loop from
	our server hosting loop along with all the variables and structures specific to it
*/
class LoopClient
{
	//Stuff we need to *play* the game, as opposed to host it, except our net interface itself
	ClientProgramData pd;
	//Network connection manager, its methods take ClientProgramData as a parameter, so it's separate
	//This could be nullptr so be careful, cmdArgs should be NotInGame if that's the case as well
	Client* client = nullptr;

	bool valid = false;

public:

	//Constructor have any issues?
	bool isValid() const { return valid; }

	//Per land of dran kino agent special request
	//Technically some UI specific calculations might happen during rendering, oh well
	void renderEverything(float deltaT);

	void handleInput(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);

	void run(float deltaT,ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);

	LoopClient(ExecutableArguments & cmdArgs,std::shared_ptr<SettingManager> settings);
	~LoopClient();
};
