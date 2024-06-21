#include "LoopClient.h"

void LoopClient::handleInput(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
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
		Logger::setDebug(settings->getBool("logger/verbose"));
		camera->updateSettings(settings);
		gui->updateSettings(settings);
	}

	//Various keys were pressed that were bound to certain commands:
	if (input->pollCommand(MouseLock))
		context->setMouseLock(!context->getMouseLocked());

	camera->control(deltaT, input);
	debugMenu->passDetails(camera);
}

void LoopClient::renderEverything(float deltaT)
{
	//Start rendering to screen:
	camera->render(shaders);

	context->select();
	context->clear(0.2f, 0.2f, 0.2f);

	shaders->modelShader->use();

	gui->render();

	context->swap();
}

void LoopClient::run(float deltaT,ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	client->run(); //  <--- networking
	handleInput(deltaT,cmdArgs,settings);
	renderEverything(deltaT);
}

LoopClient::LoopClient(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	//This will also populate key-bind specific defaults, so we reexport after
	input = std::make_shared<InputMap>(settings); 
	settings->exportToFile("Config/settings.txt");

	info("Not dedicated, initalizing client.");

	//Create our program window
	context = std::make_shared<RenderContext>(settings);
	gui = std::make_shared<UserInterface>();
	gui->updateSettings(settings);
	settingsMenu = gui->createWindow<SettingsMenu>(settings, input);
	debugMenu = gui->createWindow<DebugMenu>();
	serverBrowser = gui->createWindow<ServerBrowser>();
	serverBrowser->open();

	gui->initAll();

	//Load all the shaders
	shaders = std::make_shared<ShaderManager>();
	shaders->readShaderList("Shaders/shadersList.txt");

	camera = std::make_shared<Camera>(context->getResolution().x / context->getResolution().y);
	camera->updateSettings(settings);

	//A few test decals
	textures = std::make_shared<TextureManager>();
	textures->allocateForDecals(128);
	textures->finalizeDecals();

	client = new Client();
	if (!client->isValid())
		return;
}

LoopClient::~LoopClient()
{
	context.reset();
	gui.reset(); //Will handle indivdual windows
	shaders.reset();
	camera.reset();
	textures.reset();
	delete client;
}
