// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "GameLoop/LoopClient.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"
#include "Networking/Server.h"

int main(int argc, char* argv[])
{
	//This holds everything we need to *play* the game instead of hosting it
	//is nullptr if dedicated server
	LoopClient* loopClient = nullptr;

	//and the one thing we will have if we *are* dedicated but *aren't* playing
	Server* server = nullptr;

	Logger::setErrorFile("Logs/error.txt");
	Logger::setInfoFile("Logs/log.txt");
	info("Starting Land of Dran");

	//Parse command line arguments to see if we're hosting dedicated or launching the full game
	ExecutableArguments cmdArgs(argc, argv);

	//Import settings we have, set defaults for settings we don't, then export the file with any new default values
	info("Opening settings file");
	auto settings = std::make_shared<SettingManager>("Config/settings.txt");
	populateDefaults(settings);
	settings->exportToFile("Config/settings.txt");

	Logger::setDebug(settings->getBool("logger/verbose"));

	//Start SDL and other libraries
	globalStartup(settings,cmdArgs);

	//Start up for dedicated server or the actual game itself
	if (!cmdArgs.dedicated)
		loopClient = new LoopClient(cmdArgs,settings);
	else 
	{
		info("Dedicated flag detected, starting server.");

		server = new Server(DEFAULT_PORT);
		if (!server->isValid())
			return 0;
	}

	float lastTicks = (float)SDL_GetTicks();
	
	//The main loop of the whole program
	while (cmdArgs.mainLoopRun)
	{
		//Milliseconds since last frame
		float deltaT = ((float)SDL_GetTicks()) - lastTicks;
		lastTicks = (float)SDL_GetTicks();

		//Run server, or run game
		if (cmdArgs.dedicated)
			server->run();
		else
			loopClient->run(deltaT,cmdArgs,settings);
	}

	//Deallocate client if this wasn't a dedicated server
	if(!cmdArgs.dedicated)
		delete loopClient;

	globalShutdown(cmdArgs);
	
	return 0;
}
