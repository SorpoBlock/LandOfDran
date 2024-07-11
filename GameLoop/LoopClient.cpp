#include "LoopClient.h"

void LoopClient::leaveServer(ExecutableArguments& cmdArgs)
{
	info("Leaving server");

	pd.serverBrowser->open();

	if (!client)
		return;

	//Will need to log in again to get eval access
	pd.debugMenu->reset();

	if (simulation.dynamics)
	{
		delete simulation.dynamics;
		simulation.dynamics = nullptr;
	}

	delete client;
	client = nullptr;

	//It's very possible some or all data structures may not have been initialized or allocated if we disconnected in the middle loading into a new server

	simulation.dynamicTypes.clear();
	pd.signals.typesToLoad = 0; //Disable progress bar in server browser UI until next join
	
	simulation.evalPassword = "";

	//Destroy server specific physics
	if (pd.physicsWorld)
	{
		pd.physicsWorld.reset();
		SimObject::world = nullptr;
	}

	pd.context->setMouseLock(false);

	cmdArgs.gameState = NotInGame;
}

void LoopClient::connectToServer(std::string ip, unsigned int port, std::string userName, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (cmdArgs.gameState != NotInGame)
		leaveServer(cmdArgs);

	//TODO: Note without some kind of multithreading, this message will never display in the UI
	pd.serverBrowser->setConnectionNote("Connecting to server...");
	info("Attempting connection to " + ip + ":" + std::to_string(port));

	//TODO: Make it so I can do this without having to re-write descriptions
	settings->addString("network/username", userName, true, "Guest name if not logged in");
	settings->addString("network/lastip", ip, true, "Last IP connected to");
	settings->addInt("network/port", port, true, "Connection port");
	settings->exportToFile("Config/settings.txt");

	cmdArgs.gameState = Connecting;
	client = new Client(ip, port, settings->getInt("network/packetholdtime"));

	//Connection to server failed
	if (!client->isValid())
	{
		pd.serverBrowser->setConnectionNote("Could not connect");
		cmdArgs.gameState = NotInGame;
		delete client;
		client = nullptr;
		return;
	}

	client->send(makeConnectionRequest(userName), JoinNegotiation);
	
	//From here, further initalization will actually take place in Networking/PacketsFromServer/AcceptConnection.cpp
	//Assuming the server lets us join, of course
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
			leaveServer(cmdArgs);
			cmdArgs.mainLoopRun = false;
			break;
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				pd.context->setSize(e.window.data1, e.window.data2);
				simulation.camera->setAspectRatio(pd.context->getResolution().x / pd.context->getResolution().y);
			}
		}
		else if (e.type == SDL_MOUSEMOTION && pd.context->getMouseLocked())
			simulation.camera->turn(-(float)e.motion.xrel, -(float)e.motion.yrel);
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
		simulation.camera->updateSettings(settings);
		pd.gui->updateSettings(settings);
		simulation.idealBufferSize = settings->getInt("network/snapshotbuffer");
	}

	if (pd.debugMenu->passwordSubmitted())
	{
		std::string password = pd.debugMenu->getPassword();
		simulation.evalPassword = password;
		if (cmdArgs.gameState != InGame)
			pd.debugMenu->adminLoginComment = "Not in a server!";
		else
			client->send(attemptEvalLogin(password), OtherReliable);
	}

	if (pd.debugMenu->isCommandWaiting())
	{
		std::string command = pd.debugMenu->getLuaCommand();
		client->send(evalCommand(simulation.evalPassword, command), OtherReliable);
	}

	//Various keys were pressed that were bound to certain commands:
	if (pd.input->pollCommand(MouseLock))
		pd.context->setMouseLock(!pd.context->getMouseLocked());
	
	//Move camera around
	simulation.camera->control(deltaT, pd.input);

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
				leaveServer(cmdArgs);
			cmdArgs.mainLoopRun = false;
			return;
		}

		case LeaveServer:
		{
			if (cmdArgs.gameState != NotInGame)
				leaveServer(cmdArgs);
			break;
		}

		case None:
		default:
			break;
	}
}

void LoopClient::renderEverything(float deltaT)
{
	//TODO: Get rid of this
	if (simulation.dynamics)
	{
		for (unsigned a = 0; a < simulation.dynamics->size(); a++)
			simulation.dynamics->get(a)->updateSnapshot();
	}

	//Technically rendering related calculations based on previously inputted transform data
	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->getModel()->updateAll(deltaT);

	//Start rendering to screen:
	simulation.camera->render(pd.shaders);

	pd.context->select();
	pd.context->clear(0.2f, 0.2f, 0.2f);

	pd.shaders->modelShader->use();

	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->render(pd.shaders);

	pd.gui->render();

	pd.context->swap();
}

void LoopClient::run(float deltaT,ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (client)
	{
		KickReason reason = client->run(pd, simulation, cmdArgs); //  <--- networking, process packets
		if(reason != NotKicked) 
		{
			//We lost connection somehow
			leaveServer(cmdArgs);

			//Show pop-up
			pd.serverBrowser->setKickReason(reason);
		}
	}

	handleInput(deltaT,cmdArgs,settings); //mouse and keyboard input

	// --- UI Updates and Requests ---

	//Send info to debug menu for display
	NetInfo netInfo;
	if (client)
		netInfo = { client->getPing(), client->getIncoming(), client->getOutgoing() };
	pd.debugMenu->passDetails(simulation.camera->getPosition(),simulation.camera->getDirection(), netInfo);

	if (pd.chatWindow->hasChatMessage())
	{
		ENetPacket *chat = makeChatMessage(pd.chatWindow->getChatMessage());
		client->send(chat, OtherReliable);
	}

	//Progress loading SimObject types
	pd.serverBrowser->passLoadProgress(pd.signals.typesToLoad, simulation.dynamicTypes.size());

	// --- State changes requested from packets ---

	//Phase one loading started
	if (pd.signals.startPhaseOneLoading)
	{ 
		//Server accepted join request
		//Start up systems needed to play
		pd.physicsWorld = std::make_shared<PhysicsWorld>();
		SimObject::world = pd.physicsWorld;
		cmdArgs.gameState = LoadingTypes; 
	}

	//Phase one loading done
	if (pd.signals.finishedPhaseOneLoading)
	{
		//Create holders for objects now that we will start receiving data about them
		simulation.dynamics = new ObjHolder<Dynamic>(DynamicTypeId);

		ENetPacket* finishedLoading = makeLoadingFinished();
		client->send(finishedLoading, OtherReliable);

		cmdArgs.gameState = InGame;

		pd.serverBrowser->setConnectionNote("");
		pd.serverBrowser->close();
	}

	//All signals from packets processed for this frame, reset flags
	pd.signals.reset();

	// --- End packet requests ---

	if (pd.physicsWorld)
		pd.physicsWorld->step(deltaT);

	renderEverything(deltaT);
}

LoopClient::LoopClient(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	//This will also populate key-bind specific defaults, so we reexport after
	pd.input = std::make_shared<InputMap>(settings);
	settings->exportToFile("Config/settings.txt");

	info("Not dedicated, initalizing client.");

	simulation.idealBufferSize = settings->getInt("network/snapshotbuffer");

	//Create our program window
	pd.context = std::make_shared<RenderContext>(settings);
	pd.gui = std::make_shared<UserInterface>();
	pd.gui->updateSettings(settings);
	pd.settingsMenu = pd.gui->createWindow<SettingsMenu>(settings, pd.input);
	pd.debugMenu = pd.gui->createWindow<DebugMenu>();
	pd.escapeMenu = pd.gui->createWindow<EscapeMenu>();
	pd.serverBrowser = pd.gui->createWindow<ServerBrowser>();
	pd.chatWindow = pd.gui->createWindow<ChatWindow>();
	pd.serverBrowser->passDefaultSettings(settings->getString("network/lastip"), settings->getInt("network/port"), settings->getString("network/username"));
	pd.serverBrowser->open();

	pd.gui->initAll();

	//Load all the shaders
	pd.shaders = std::make_shared<ShaderManager>();
	pd.shaders->readShaderList("Shaders/shadersList.txt");

	simulation.camera = std::make_shared<Camera>(pd.context->getResolution().x / pd.context->getResolution().y);
	simulation.camera->updateSettings(settings);

	//A few test decals
	pd.textures = std::make_shared<TextureManager>();
	pd.textures->allocateForDecals(128);
	pd.textures->finalizeDecals();

	info("Start up complete");

	valid = true;
}

LoopClient::~LoopClient()
{
	//Not needed this is a destructor lol
	//Also this should only be called when the programs shutting down anyway 
	pd.context.reset();
	pd.gui.reset(); //Will handle indivdual windows
	pd.shaders.reset();
	pd.textures.reset();

	//This one is actually useful because the server will learn we disconnected faster if we do it properly 
	if(client)
		delete client;
}
