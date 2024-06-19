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

#include "Utility/UserInterfaceFunctions.h"
#include "Interface/DebugMenu.h"

using namespace std;

int main(int argc, char* argv[])
{
	/*
		Holds everything we need for playing the game
		Not needed if this is a dedicated server
		Everything inside should be allocated and populated otherwise
		Nothing should be removed or deallocated until program shutdown
	*/
	
	Logger::setErrorFile("Logs/error.txt");
	Logger::setInfoFile("Logs/log.txt");

	info("Starting Land of Dran");  

	info("Opening preferences file");

	//Import settings we have, set defaults for settings we don't, then export the file with any new default values
	auto preferences = std::make_shared<SettingManager>("Config/settings.txt");
	populateDefaults(preferences);
	auto input = std::make_shared<InputMap>(preferences); //This will also populate key-bind specific defaults
	preferences->exportToFile("Config/settings.txt");

	Logger::setDebug(preferences->getBool("logger/verbose"));

	//Start SDL and other libraries
	globalStartup(preferences);

	//Create our program window
	RenderContext context(preferences);

	UserInterface gui;
	gui.globalInterfaceTransparency = preferences->getFloat("gui/opacity");

	auto settingsMenu = std::make_shared<SettingsMenu>(preferences,input);
	gui.addWindow(settingsMenu);

	auto debugMenu = std::make_shared<DebugMenu>();
	gui.addWindow(debugMenu);

	gui.initAll();

	//Load all the shaders
	ShaderManager shaders;
	shaders.readShaderList("Shaders/shadersList.txt");

	auto camera = std::make_shared<Camera>(context.getResolution().x / context.getResolution().y);
	camera->mouseSensitivity = preferences->getFloat("input/mousesensitivity");
	camera->invertMouse = preferences->getFloat("input/invertmousey");

	//A few test decals
	TextureManager textures;
	textures.allocateForDecals(128);
	textures.addDecal("Assets/animan.png", 0);
	textures.addDecal("Assets/ascii-terror.png", 1);
	textures.finalizeDecals();

	//Test model
	Model testModel("Assets/brickhead/brickhead.txt",&textures); 
	testModel.baseScale = glm::vec3(0.01);
	testModel.setDefaultFrame(35);

	//Test animations
	Animation walk;
	walk.defaultSpeed = 0.01;
	walk.startTime = 0;
	walk.endTime = 30;
	walk.serverID = 0;
	walk.name = "walk";
	walk.fadeInMS = 200;
	walk.fadeOutMS = 400;
	testModel.addAnimation(walk);

	Animation grab;
	grab.defaultSpeed = 0.01;
	grab.startTime = 57;
	grab.endTime = 66;
	grab.serverID = 1;
	grab.name = "grab";
	grab.fadeInMS = 200;
	grab.fadeOutMS = 400;
	testModel.addAnimation(grab);

	//Alternative test model and animations
	/*Model test("Assets/gun/gun.txt", &textures);
	test.setDefaultFrame(25);

	Animation fire;
	fire.defaultSpeed = 0.01;
	fire.startTime = 39;
	fire.endTime = 86;
	fire.serverID = 0;
	fire.name = "fire";
	test.addAnimation(fire);

	Animation reload;
	reload.defaultSpeed = 0.01;
	reload.startTime = 1;
	reload.endTime = 21;
	reload.serverID = 1;
	reload.name = "fire";
	test.addAnimation(reload);*/

	//Spawn in a ton of instances of the test model
	std::vector<ModelInstance*> instances;
	for (unsigned int a = 0; a < 7; a++)
	{
		for (unsigned int b = 0; b < 7; b++)
		{
			for (unsigned int c = 0; c < 7; c++)
			{
				ModelInstance* tester = new ModelInstance(&testModel);
				instances.push_back(tester);
				tester->setModelTransform(glm::translate(glm::vec3(a * 8, b * 8, c * 8)));
				tester->update(0);
			} 
		}
	} 

	float angle = 0.0;
	float lastTicks = SDL_GetTicks();

	bool doMainLoop = true;
	while (doMainLoop)
	{
		float deltaT = ((float)SDL_GetTicks()) - lastTicks;
		lastTicks = SDL_GetTicks();
		angle += deltaT * 0.001;

		input->keystates = SDL_GetKeyboardState(NULL);

		//Event loop, mostly just passing stuff to InputMap (in-game controls) and UserInterface (gui controls)
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			settingsMenu->processKeyBind(e);
			gui.handleInput(e);
			input->handleInput(e);

			if (e.type == SDL_QUIT)
			{
				doMainLoop = false;
				break;
			}
			else if (e.type == SDL_WINDOWEVENT)
			{
				if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					context.setSize(e.window.data1, e.window.data2);
					camera->setAspectRatio(context.getResolution().x / context.getResolution().y);
				}
			}
			else if (e.type == SDL_MOUSEMOTION && context.getMouseLocked())
				camera->turn(-e.motion.xrel, -e.motion.yrel);
			else if (e.type == SDL_MOUSEBUTTONDOWN)
				instances[0]->playAnimation(1, false);
			else if (e.type == SDL_KEYDOWN)
			{
				//This one is not handled through input map because input map can be suppressed
				//By having one or more guis open, which defeats the purpose of a quick gui close key
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					gui.closeOneWindow();
					if (!context.getMouseLocked() && !gui.getOpenWindowCount())
						context.setMouseLock(true);
				}
			}
		}

		//Interacting with gui, don't move around in-game
		input->supressed = gui.wantsSuppression();

		//Various keys were pressed that were bound to certain commands:
		if (input->pollCommand(MouseLock))
			context.setMouseLock(!context.getMouseLocked());
		if (input->pollCommand(OpenOptionsMenu))
		{
			settingsMenu->open();
			context.setMouseLock(false);
		}
		if (input->pollCommand(OpenDebugWindow))
		{
			debugMenu->open();
			context.setMouseLock(false);
		}

		//Test camera controls, no-clip camera
		float speed = 0.015;

		if (input->isCommandKeydown(WalkForward))
			camera->flyStraight(deltaT * speed);
		if (input->isCommandKeydown(WalkBackward))
			camera->flyStraight(-deltaT * speed);
		if (input->isCommandKeydown(WalkRight))
			camera->flySideways(-deltaT * speed);
		if (input->isCommandKeydown(WalkLeft))
			camera->flySideways(deltaT * speed);

		debugMenu->passDetails(camera);

		//Start rendering to screen:
		testModel.updateAll(deltaT);
		camera->render(&shaders);

		context.select(); 
		context.clear(0.2,0.2,0.2);

		shaders.modelShader->use();
		testModel.render(&shaders);

		gui.render();

		context.swap();
	}

	globalShutdown();
	
	return 0;
}
