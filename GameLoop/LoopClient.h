#pragma once

#include "../LandOfDran.h"

#include "../Networking/Client.h"
#include "ClientProgramData.h"
#include "Simulation.h"
#include "../Networking/ClientPacketCreators.h"

/*
	This is the big bad class that allows us to separate our client playing loop from
	our server hosting loop along with all the variables and structures specific to it
*/
class LoopClient
{
	//Stuff we need to *play* the game, as opposed to host it, except our net interface itself
	ClientProgramData pd;

	//Anything with state specific to and maintained by the current server we're playing on
	Simulation simulation;

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

	//Called every frame the program runs. Every frame: in a game, not in a game, loading into a game...
	void handleInput(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);

	//Clean up performed when leaving a server to return to main menu or joining another server
	void leaveServer();

	//Joining new server from main menu or another server
	void connectToServer(std::string ip,unsigned int port,std::string userName, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);

	//Called every frame the program runs. Every frame: in a game, not in a game, loading into a game...
	void run(float deltaT,ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings);

	/*
		Only called when the program starts initially
		LoopClient is *not* created and destroyed each time you join a server
	*/
	LoopClient(ExecutableArguments & cmdArgs,std::shared_ptr<SettingManager> settings);
	//Only called when the program finally shuts down, *not* called when leaving a server
	~LoopClient();
};
