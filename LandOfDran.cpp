﻿// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"
#include "Utility/ClientData.h"


using namespace std;

int main(int argc, char* argv[])
{
	/*
		Holds everything we need for playing the game
		Not needed if this is a dedicated server
		Everything inside should be allocated and populated otherwise
		Nothing should be removed or deallocated until program shutdown
	*/
	ClientData clientEnv;
	
	Logger::setErrorFile("Logs/error.txt");
	Logger::setInfoFile("Logs/log.txt");

	info("Starting Land of Dran");  

	info("Opening preferences file");

	//Import settings we have, set defaults for settings we don't, then export the file with any new default values
	SettingManager preferences("Config/settings.txt");
	populateDefaults(preferences);
	preferences.exportToFile("Config/settings.txt");
	clientEnv.preferences = &preferences;

	Logger::setDebug(preferences.getBool("logger/verbose"));

	globalStartup(preferences);

	RenderContext context(preferences);
	clientEnv.renderContext = &context;

	bool doMainLoop = true;
	while (doMainLoop)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				doMainLoop = false;
				break;
			}
		}

		context.clear(1,1,1);
		
		context.swap();
	}

	globalShutdown();
	
	return 0;
}
