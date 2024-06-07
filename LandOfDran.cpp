// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"

using namespace std;

int main(int argc, char* argv[])
{
	Logger::setErrorFile("Logs/error.txt");
	Logger::setInfoFile("Logs/log.txt");

	info("Starting Land of Dran");  

	info("Opening preferences file");

	SettingManager preferences("Config/settings.txt");
	populateDefaults(preferences);
	preferences.exportToFile("Config/settings.txt");



	globalStartup(preferences);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow("Land of Dran", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 1024, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	GLenum glewError = glewInit();

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

		SDL_GL_SwapWindow(window);
	}

	globalShutdown();
	
	return 0;
}
