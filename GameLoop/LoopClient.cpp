#include "LoopClient.h"

void LoopClient::leaveServer(ExecutableArguments& cmdArgs)
{
	info("Leaving server");

	pd.serverBrowser->open();

	if (!client)
		return;

	pd.chatWindow->close();

	//Will need to log in again to get eval access
	pd.debugMenu->reset();

	simulation.controllers.clear();
	simulation.controlledDynamics.clear();

	if (simulation.dynamics)
	{
		delete simulation.dynamics;
		simulation.dynamics = nullptr;
	}

	if (simulation.statics)
	{
		delete simulation.statics;
		simulation.statics = nullptr;
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
		else if (e.type == SDL_MOUSEBUTTONDOWN && simulation.camera && !pd.gui->shouldUnlockMouse() && cmdArgs.gameState == InGame)
		{
			int mx, my;
			int mask = SDL_GetMouseState(&mx, &my);

			float x = (float)mx / pd.context->getResolution().x * 2 - 1;
			float y = (float)my / pd.context->getResolution().y * 2 - 1;

			glm::vec3 worldPos = simulation.camera->mouseCoordsToWorldSpace(glm::vec2(x, y));
			glm::vec3 dir = simulation.camera->getDirection();

			ENetPacket *mouseClickPacket = makeMouseClickPacket(worldPos, dir, mask);
			client->send(mouseClickPacket, OtherReliable);
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

		case OpenChat:
		{
			pd.chatWindow->open();
			break;
		}

		case OpenSettings:
		{
			pd.settingsMenu->open();
			break;
		}

		case OpenDebugMenu:
		{
			pd.debugMenu->open();
			break;
		}

		case None:
		default:
			break;
	}

	if (pd.input->pollCommand(FirstThirdPerson))
		simulation.camera->swapPerson();
}

void LoopClient::renderEverything(float deltaT)
{
	//TODO: Get rid of this
	if (simulation.dynamics)
	{
		for (unsigned a = 0; a < simulation.dynamics->size(); a++)
			simulation.dynamics->get(a)->updateSnapshot(pd.input->isCommandKeydown(DebugView));
	}

	//Technically rendering related calculations based on previously inputted transform data
	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->getModel()->updateAll(deltaT);

	simulation.camera->calculateLightSpaceMatricies(glm::normalize(glm::vec3(0.2, 1.0, 0.4)), pd.lightSpaceMatricies);

	//Render shadows to texture:
	pd.shadows->use();
	pd.shaders->modelShadowShader->use();
	glUniformMatrix4fv(pd.lightSpaceMatriciesUniformShadow, 3, GL_FALSE, (GLfloat*)pd.lightSpaceMatricies);

	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->render(pd.shaders,false);

	//Start rendering to screen:
	pd.context->select();
	pd.context->clear(0.4f, 0.4f, 0.8f);

	simulation.camera->render(pd.shaders, deltaT, pd.physicsWorld);

	pd.shaders->modelShader->use();
	glUniformMatrix4fv(pd.lightSpaceMatriciesUniformModel, 3, GL_FALSE, (GLfloat*)pd.lightSpaceMatricies);
	pd.shadows->bindDepthResult(ShadowArray);
	pd.shaders->basicUniforms.nonInstanced = 0;
	pd.shaders->basicUniforms.cameraSpacePosition = 0;

	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->render(pd.shaders);

	//Render grass
	pd.shaders->basicUniforms.ScaleMatrix = glm::mat4(1.0);
	pd.shaders->basicUniforms.TranslationMatrix = glm::mat4(1.0);
	pd.shaders->basicUniforms.RotationMatrix = glm::mat4(1.0);
	pd.shaders->basicUniforms.nonInstanced = 1;
	pd.shaders->basicUniforms.cameraSpacePosition = 1;
	pd.grassMaterial->use(pd.shaders);
	glBindVertexArray(pd.grassVao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	bool crossHair = false;
	if (simulation.camera)
		crossHair = pd.context->getMouseLocked() && simulation.camera->getFirstPerson();
	pd.gui->render(pd.context->getResolution().x, pd.context->getResolution().y,crossHair);

	pd.context->swap();
}

void LoopClient::sendControlledObjects()
{
	if(getTicksMS() - lastSentControlledObjects < 100)
		return;

	lastSentControlledObjects = getTicksMS();

	//We're simulating these objects for the server, send updates on their transforms back to the server
	for (int i = 0; i < simulation.controlledDynamics.size(); i++)
	{
		std::shared_ptr<Dynamic> d = simulation.controlledDynamics[i];

		//TODO: Do this through objHolder I guess? Ideally there'd never be more than like 1 or 2 of these per client tho
		if (!d->requiresNetUpdate())
			continue;

		ENetPacket* update = enet_packet_create(NULL, d->getUpdatePacketBytes() + 1 + sizeof(netIDType), getFlagsFromChannel(Unreliable));
		update->data[0] = (unsigned char)ControlledPhysics;
		netIDType id = d->getID();
		memcpy(update->data + 1, &id, sizeof(netIDType));
		d->addToUpdatePacket(update->data + 1 + sizeof(netIDType));
		client->send(update, Unreliable);
	}
}

void LoopClient::updateControllers(float deltaT)
{
	//Go through player controllers, remove any that are bound to now deleted dynamics 
	auto ctrlIter = simulation.controllers.begin();
	while (ctrlIter != simulation.controllers.end())
	{
		//Apply movement inputs client side 
		if ((*ctrlIter)->control(pd.input, simulation.camera, deltaT, pd.physicsWorld))
		{
			ctrlIter = simulation.controllers.erase(ctrlIter);
			continue;
		}
		else
		{
			//Send movement inputs to server to be applied there
			if (!client)
			{
				++ctrlIter;
				continue;
			}

			ENetPacket* packet = (*ctrlIter)->makeMovementInputsPacket();
			if(packet)
				client->send(packet, Unreliable);

			++ctrlIter;
		}
	}
}

void LoopClient::run(float deltaT,ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (client)
	{
		//We're in game, equivalent to gameState == InGame

		KickReason reason = client->run(pd, simulation, cmdArgs); //  <--- networking, process packets
		if(reason != NotKicked) 
		{
			//We lost connection somehow
			leaveServer(cmdArgs);

			//Show pop-up
			pd.serverBrowser->setKickReason(reason);
		}

		sendControlledObjects();
	}

	//movement keys and camera direction as it relates to players / controlled objects 
	updateControllers(deltaT); 

	handleInput(deltaT,cmdArgs,settings); //mouse and keyboard input

	// --- UI Updates and Requests ---

	//Send info to debug menu for display
	NetInfo netInfo;
	if (client)
		netInfo = { client->getPing(), client->getIncoming(), client->getOutgoing(), simulation.serverLastSlowestFrame, simulation.serverAverageFrame };
	pd.debugMenu->passDetails(simulation.camera->getPosition(),simulation.camera->getDirection(), netInfo);

	if (simulation.dynamics && simulation.dynamics->size() > 0)
		pd.debugMenu->addExtraLine("First other snaps: " + std::to_string(simulation.dynamics->get(0)->interpolator.getNumSnapshots()));
	if (simulation.controlledDynamics.size() > 0)
		pd.debugMenu->addExtraLine("First controlled snaps: " + std::to_string(simulation.controlledDynamics[0]->interpolator.getNumSnapshots()));
	if (client)
	{
		pd.debugMenu->addExtraLine("Total queued packets: " + std::to_string(client->getNumQueued()));
		pd.debugMenu->addExtraLine("Ping variance: " + std::to_string(client->getPingVariance()));
		pd.debugMenu->addExtraLine("Packet loss: " + std::to_string(client->getLoss()));
	}

	if (pd.chatWindow->hasChatMessage() && client)
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
		simulation.statics = new ObjHolder<StaticObject>(StaticTypeId);

		ENetPacket* finishedLoading = makeLoadingFinished();
		client->send(finishedLoading, OtherReliable);

		cmdArgs.gameState = InGame;

		pd.serverBrowser->setConnectionNote("");
		pd.serverBrowser->close();

		pd.chatWindow->open();
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
	if(pd.shaders->readShaderList("Shaders/shadersList.txt"))
	{
		pd.gui->popupErrorMessage = "Error loading shaders, see error log.";
	}

	simulation.camera = std::make_shared<Camera>(pd.context->getResolution().x / pd.context->getResolution().y);
	simulation.camera->updateSettings(settings);

	//A few test decals
	pd.textures = std::make_shared<TextureManager>();
	pd.textures->allocateForDecals(128);
	pd.textures->finalizeDecals();

	pd.grassMaterial = new Material("Assets/ground/grass.txt", pd.textures);

	if (!pd.grassMaterial->isValid())
	{
		pd.gui->popupErrorMessage = "Error loading grass material, see error log.";
	}

	pd.grassVao = createQuadVAO();

	RenderTarget::RenderTargetSettings shadowSettings;
	shadowSettings.width = 2048;
	shadowSettings.height = 2048;
	shadowSettings.layers = 3;
	shadowSettings.useColor = false;
	pd.shadows = std::make_shared<RenderTarget>(shadowSettings,pd.textures);
	pd.lightSpaceMatriciesUniformShadow = pd.shaders->modelShadowShader->getUniformLocation("lightSpaceMatricies");
	pd.lightSpaceMatriciesUniformModel = pd.shaders->modelShader->getUniformLocation("lightSpaceMatricies");

	info("Start up complete");

	printAllGraphicsErrors("End of initalization");

	valid = true;
}

LoopClient::~LoopClient()
{
	pd.shadows.reset();

	delete pd.grassMaterial;
	glDeleteVertexArrays(1, &pd.grassVao);

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
