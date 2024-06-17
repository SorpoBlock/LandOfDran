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

using namespace std;

// Our state
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void DrawExampleWindow(ImGuiIO& io)
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}

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
	InputMap input(preferences); //This will also populate key-bind specific defaults
	preferences->exportToFile("Config/settings.txt");

	Logger::setDebug(preferences->getBool("logger/verbose"));

	//Start SDL and other libraries
	globalStartup(preferences);

	//Create our program window
	RenderContext context(preferences);

	UserInterface gui;
	gui.globalInterfaceTransparency = preferences->getFloat("gui/opacity");

	auto settingsMenu = std::make_shared<SettingsMenu>(preferences);
	gui.addWindow(settingsMenu);

	//Load all the shaders
	ShaderManager shaders;
	shaders.readShaderList("Shaders/shadersList.txt");

	Camera camera(context.getResolution().x / context.getResolution().y);
	camera.mouseSensitivity = preferences->getFloat("input/mousesensitivity");
	camera.invertMouse = preferences->getFloat("input/invertmousey");

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

		input.keystates = SDL_GetKeyboardState(NULL);

		//TODO: This is used for making sure no call to gui.handleInput wants to suppress
		//but really you should only have to call a function once to get this
		bool shouldSuppress = false;

		//Event loop, mostly just passing stuff to InputMap (in-game controls) and UserInterface (gui controls)
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			shouldSuppress = gui.handleInput(e) || shouldSuppress;
			input.handleInput(e);

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
					camera.setAspectRatio(context.getResolution().x / context.getResolution().y);
				}
			}
			else if (e.type == SDL_MOUSEMOTION && context.getMouseLocked())
				camera.turn(-e.motion.xrel, -e.motion.yrel);
			else if (e.type == SDL_MOUSEBUTTONDOWN)
				instances[0]->playAnimation(1, false);
		}

		//Interacting with gui, don't move around in-game
		input.supressed = shouldSuppress;

		//Windows are open, let mouse move around
		if (context.getMouseLocked() && gui.shouldUnlockMouse())
			context.setMouseLock(false);

		//Various keys were pressed that were bound to certain commands:
		if (input.pollCommand(CloseWindow))
		{
			gui.closeOneWindow();
			if (!context.getMouseLocked() && !gui.getOpenWindowCount())
				context.setMouseLock(true);
		}
		if (input.pollCommand(MouseLock))
			context.setMouseLock(!context.getMouseLocked());
		if (input.pollCommand(OptionsMenu))
			settingsMenu->open();

		//Test camera controls, no-clip camera
		float speed = 0.015;

		if (input.isCommandKeydown(WalkForward))
			camera.flyStraight(deltaT * speed);
		if (input.isCommandKeydown(WalkBackward))
			camera.flyStraight(-deltaT * speed);
		if (input.isCommandKeydown(WalkRight))
			camera.flySideways(-deltaT * speed);
		if (input.isCommandKeydown(WalkLeft))
			camera.flySideways(deltaT * speed);

		//Start rendering to screen:
		testModel.updateAll(deltaT);
		camera.render(&shaders);

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
