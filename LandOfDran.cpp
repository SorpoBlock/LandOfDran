// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "GameLoop/LoopClient.h"
#include "GameLoop/LoopServer.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"

int main(int argc, char* argv[])
{
	//This holds everything we need to *play* the game instead of hosting it
	//is nullptr if dedicated server
	LoopClient* loopClient = nullptr;

	//Everything needed to *host* the game instead of playing it
	//is nullptr if not dedicated server
	LoopServer* loopServer = nullptr;

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
	if (cmdArgs.dedicated)
	{
		info("Dedicated flag detected, starting server.");

		loopServer = new LoopServer(cmdArgs, settings);
		if (!loopServer->isValid())
			return 0;
	}
	else 
	{
		loopClient = new LoopClient(cmdArgs, settings);
		if (!loopClient->isValid())
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
			loopServer->run(deltaT,cmdArgs,settings);
		else
			loopClient->run(deltaT,cmdArgs,settings);
	}

	//Deallocate client if this wasn't a dedicated server
	if (cmdArgs.dedicated)
		delete loopServer;
	else
		delete loopClient;

	globalShutdown(cmdArgs);
	
	return 0;
}
