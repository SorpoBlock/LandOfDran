// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"
#include "Graphics/ShaderSpecification.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "Graphics/RenderContext.h"
#include "Interface/InputMap.h"
#include "Graphics/PlayerCamera.h"
#include "Interface/SettingsMenu.h"
#include "Interface/DebugMenu.h"
#include "Networking/Client.h"
#include "Networking/Server.h"

int main(int argc, char* argv[])
{
	//Here's a bunch of things that *might* be used if we're not running dedicated...
	std::shared_ptr<RenderContext>	context = nullptr;
	std::shared_ptr<UserInterface>	gui = nullptr;
	std::shared_ptr<SettingsMenu>	settingsMenu = nullptr;
	std::shared_ptr<DebugMenu>		debugMenu = nullptr;
	std::shared_ptr<ShaderManager>	shaders = nullptr;
	std::shared_ptr<Camera>			camera = nullptr;
	std::shared_ptr<TextureManager> textures = nullptr;
	std::shared_ptr<InputMap>		input = nullptr;
	Client* client = nullptr;

	//...and the one thing we will have if we *are* dedicated but *aren't* playing
	Server* server = nullptr;

	ExecutableArguments cmdArgs(argc, argv);

	Logger::setErrorFile("Logs/error.txt");
	Logger::setInfoFile("Logs/log.txt");

	info("Starting Land of Dran");  

	info("Opening preferences file");

	//Import settings we have, set defaults for settings we don't, then export the file with any new default values
	auto preferences = std::make_shared<SettingManager>("Config/settings.txt");
	populateDefaults(preferences);
	if(!cmdArgs.dedicated)
		input = std::make_shared<InputMap>(preferences); //This will also populate key-bind specific defaults
	preferences->exportToFile("Config/settings.txt");

	Logger::setDebug(preferences->getBool("logger/verbose"));

	//Start SDL and other libraries
	globalStartup(preferences,cmdArgs);

	//Running the game
	if (!cmdArgs.dedicated)
	{
		//Create our program window
		context = std::make_shared<RenderContext>(preferences);
		gui = std::make_shared<UserInterface>();
		gui->updateSettings(preferences);
		settingsMenu = gui->createWindow<SettingsMenu>(preferences, input);
		debugMenu = gui->createWindow<DebugMenu>();

		gui->initAll();

		//Load all the shaders
		shaders = std::make_shared<ShaderManager>();
		shaders->readShaderList("Shaders/shadersList.txt");

		camera = std::make_shared<Camera>(context->getResolution().x / context->getResolution().y);
		camera->updateSettings(preferences);

		//A few test decals
		textures = std::make_shared<TextureManager>();
		textures->allocateForDecals(128);
		textures->finalizeDecals();

		client = new Client();
		if (!client->isValid())
			return 0;
	}
	//Running the dedicated headless server
	else
	{
		server = new Server(DEFAULT_PORT);
		if (!server->isValid())
			return 0;
	}

	float lastTicks = (float)SDL_GetTicks();
	
	while (cmdArgs.mainLoopRun)
	{
		float deltaT = ((float)SDL_GetTicks()) - lastTicks;
		lastTicks = (float)SDL_GetTicks();

		if (cmdArgs.dedicated)
			server->run();
		else
		{
			client->run();

			input->keystates = SDL_GetKeyboardState(NULL);

			//Event loop, mostly just passing stuff to InputMap (in-game controls) and UserInterface (gui controls)
			SDL_Event e;
			while (SDL_PollEvent(&e))
			{
				if (gui->handleInput(e, input))
					context->setMouseLock(false);
				input->handleInput(e);

				if (e.type == SDL_QUIT)
				{
					cmdArgs.mainLoopRun = false;
					break;
				}
				else if (e.type == SDL_WINDOWEVENT)
				{
					if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					{
						context->setSize(e.window.data1, e.window.data2);
						camera->setAspectRatio(context->getResolution().x / context->getResolution().y);
					}
				}
				else if (e.type == SDL_MOUSEMOTION && context->getMouseLocked())
					camera->turn(-(float)e.motion.xrel, -(float)e.motion.yrel);
				else if (e.type == SDL_KEYDOWN)
				{
					client->testSend();

					//This one is not handled through input map because input map can be suppressed
					//By having one or more guis open, which defeats the purpose of a quick gui close key
					if (e.key.keysym.sym == SDLK_ESCAPE)
					{
						gui->closeOneWindow();
						if (!context->getMouseLocked() && !gui->getOpenWindowCount())
							context->setMouseLock(true);
					}
				}
			}

			//Interacting with gui, don't move around in-game
			input->supressed = gui->wantsSuppression();

			//Someone just applied setting changes
			if (settingsMenu->pollForChanges())
			{
				Logger::setDebug(preferences->getBool("logger/verbose"));
				camera->updateSettings(preferences);
				gui->updateSettings(preferences);
			}

			//Various keys were pressed that were bound to certain commands:
			if (input->pollCommand(MouseLock))
				context->setMouseLock(!context->getMouseLocked());

			camera->control(deltaT, input);
			debugMenu->passDetails(camera);

			//Start rendering to screen:
			camera->render(shaders);

			context->select();
			context->clear(0.2f, 0.2f, 0.2f);

			shaders->modelShader->use();

			gui->render();

			context->swap();
		}
	}

	globalShutdown(cmdArgs);
	
	return 0;
}
