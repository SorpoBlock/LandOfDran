#include "LoopClient.h"

void LoopClient::leaveServer()
{
	info("Leaving server");

	pd.serverBrowser->open();

	if (!client)
		return;

	delete client;
	client = nullptr;
}

void LoopClient::connectToServer(std::string ip, unsigned int port, std::string userName, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (cmdArgs.gameState != NotInGame)
		leaveServer();

	//TODO: Note without some kind of multithreading, this message will never display
	pd.serverBrowser->setConnectionNote("Connecting to server...");

	cmdArgs.gameState = Connecting;
	client = new Client(ip, port, settings->getInt("network/packetholdtime"));

	info("Attempting connection to " + ip + ":" + std::to_string(port));

	//Connection to server failed
	if (!client->isValid())
	{
		pd.serverBrowser->setConnectionNote("Could not connect");
		cmdArgs.gameState = NotInGame;
		delete client;
		client = nullptr;
		return;
	}

	//Initial connection to server succeeded
	pd.serverBrowser->close();

	client->send(makeConnectionRequest(userName), JoinNegotiation);
}

void LoopClient::handleInput(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	pd.input->keystates = SDL_GetKeyboardState(NULL);

	//Event loop, mostly just passing stuff to InputMap (in-game controls) and UserInterface (gui controls)
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		if (pd.gui->handleInput(e, pd.input))
			pd.context->setMouseLock(false);
		pd.input->handleInput(e);

		if (e.type == SDL_QUIT)
		{
			cmdArgs.mainLoopRun = false;
			break;
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				pd.context->setSize(e.window.data1, e.window.data2);
				pd.camera->setAspectRatio(pd.context->getResolution().x / pd.context->getResolution().y);
			}
		}
		else if (e.type == SDL_MOUSEMOTION && pd.context->getMouseLocked())
			pd.camera->turn(-(float)e.motion.xrel, -(float)e.motion.yrel);
		else if (e.type == SDL_KEYDOWN)
		{
			//This one is not handled through input map because input map can be suppressed
			//By having one or more guis open, which defeats the purpose of a quick gui close key
			if (e.key.keysym.sym == SDLK_ESCAPE)
			{
				if (pd.gui->getOpenWindowCount() == 0)
				{
					pd.escapeMenu->open();
					pd.context->setMouseLock(false);
				}
				else
				{
					pd.gui->closeOneWindow();
					if (!pd.context->getMouseLocked() && !pd.gui->getOpenWindowCount())
						pd.context->setMouseLock(true);
				}
			}
		}
	}

	//Interacting with gui, don't move around in-game
	pd.input->supressed = pd.gui->wantsSuppression();

	//Someone just applied setting changes
	if (pd.settingsMenu->pollForChanges())
	{
		Logger::setDebug(settings->getBool("logger/verbose"));
		pd.camera->updateSettings(settings);
		pd.gui->updateSettings(settings);
	}

	//Various keys were pressed that were bound to certain commands:
	if (pd.input->pollCommand(MouseLock))
		pd.context->setMouseLock(!pd.context->getMouseLocked());

	pd.camera->control(deltaT, pd.input);
	pd.debugMenu->passDetails(pd.camera);

	if (pd.serverBrowser->serverPickReady())
	{
		std::string ip,userName;
		int port;
		pd.serverBrowser->getServerData(ip, port,userName);
		connectToServer(ip, port,userName,cmdArgs,settings);
	}

	EscapeButtonPressed escapeMenuButton = pd.escapeMenu->getLastButtonPress();
	switch (escapeMenuButton)
	{
		case LeaveGame:
		{
			if (cmdArgs.gameState != NotInGame)
				leaveServer();
			cmdArgs.mainLoopRun = false;
			return;
		}

		case LeaveServer:
		{
			if (cmdArgs.gameState != NotInGame)
				leaveServer();
			break;
		}

		case None:
		default:
			break;
	}
}

void LoopClient::renderEverything(float deltaT)
{
	//Start rendering to screen:
	pd.camera->render(pd.shaders);

	pd.context->select();
	pd.context->clear(0.2f, 0.2f, 0.2f);

	pd.shaders->modelShader->use();

	pd.gui->render();

	pd.context->swap();
}

void LoopClient::run(float deltaT,ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (client)
	{
		if (client->run(pd, cmdArgs)) //  <--- networking
			leaveServer();			  //If true, we were kicked
	}
	handleInput(deltaT,cmdArgs,settings);
	renderEverything(deltaT);
}

LoopClient::LoopClient(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	//This will also populate key-bind specific defaults, so we reexport after
	pd.input = std::make_shared<InputMap>(settings);
	settings->exportToFile("Config/settings.txt");

	info("Not dedicated, initalizing client.");

	//Create our program window
	pd.context = std::make_shared<RenderContext>(settings);
	pd.gui = std::make_shared<UserInterface>();
	pd.gui->updateSettings(settings);
	pd.settingsMenu = pd.gui->createWindow<SettingsMenu>(settings, pd.input);
	pd.debugMenu = pd.gui->createWindow<DebugMenu>();
	pd.escapeMenu = pd.gui->createWindow<EscapeMenu>();
	pd.serverBrowser = pd.gui->createWindow<ServerBrowser>();
	pd.serverBrowser->open();

	pd.gui->initAll();

	//Load all the shaders
	pd.shaders = std::make_shared<ShaderManager>();
	pd.shaders->readShaderList("Shaders/shadersList.txt");

	pd.camera = std::make_shared<Camera>(pd.context->getResolution().x / pd.context->getResolution().y);
	pd.camera->updateSettings(settings);

	//A few test decals
	pd.textures = std::make_shared<TextureManager>();
	pd.textures->allocateForDecals(128);
	pd.textures->finalizeDecals();

	info("Start up complete");

	valid = true;
}

LoopClient::~LoopClient()
{
	pd.context.reset();
	pd.gui.reset(); //Will handle indivdual windows
	pd.shaders.reset();
	pd.camera.reset();
	pd.textures.reset();
	if(client)
		delete client;
}
