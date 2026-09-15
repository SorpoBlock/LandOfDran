#include "LoopClient.h"

void LoopClient::leaveServer(ExecutableArguments& cmdArgs)
{
	info("Leaving server");

	pd.serverBrowser->open();

	if (!client)
		return;

	pd.chatWindow->close();
	pd.wrenchDialog->close();

	//Will need to log in again to get eval access
	pd.debugMenu->reset();

	simulation.controllers.clear();
	simulation.controlledDynamics.clear();

	//destroyAll actually frees each object (and its ModelInstance, removing it from e.g. the highlight list)
	//Deleting the holder alone would leak them, leaving their highlights drawn over the main menu
	if (simulation.dynamics)
	{
		simulation.dynamics->destroyAll();
		delete simulation.dynamics;
		simulation.dynamics = nullptr;
	}

	if (simulation.statics)
	{
		simulation.statics->destroyAll();
		delete simulation.statics;
		simulation.statics = nullptr;
	}

	if (simulation.lights)
	{
		simulation.lights->destroyAll();
		delete simulation.lights;
		simulation.lights = nullptr;
	}

	if (simulation.emitters)
	{
		simulation.emitters->destroyAll();
		delete simulation.emitters;
		simulation.emitters = nullptr;
	}
	pd.particles->clear();

	//Removes brick and debris bodies, so it has to happen before the physics world is destroyed below
	delete simulation.bricks;
	simulation.bricks = nullptr;
	delete simulation.brickDebris;
	simulation.brickDebris = nullptr;
	simulation.brickTypeFromServer.clear();
	simulation.brickTypeToServer.clear();

	delete client;
	client = nullptr;

	//It's very possible some or all data structures may not have been initialized or allocated if we disconnected in the middle loading into a new server

	simulation.dynamicTypes.clear();
	pd.signals.typesToLoad = 0; //Disable progress bar in server browser UI until next join
	
	simulation.evalPassword = "";
	simulation.waterEnabled = false;
	pd.waterRipples.clear();
	simulation.dayCycle = DayCycle();
	pd.ghostBrick.hide();
	pd.brickHotbar->putAway();
	pd.brickHotbar->takeChange();
	simulation.jetsEnabled = true;
	simulation.flashlightEnabled = true;
	flashlightOn = false;
	flashlightHeldMS = 0;
	flashlightCycling = false;
	pd.voice->clear();
	pd.audio->clear();

	//Destroy server specific physics
	if (pd.physicsWorld)
	{
		pd.physicsWorld.reset();
		SimObject::world = nullptr;
	}

	pd.context->setMouseLock(false);

	cmdArgs.gameState = NotInGame;

	//Shut down our embedded single player server, if there was one
	if (localServer)
	{
		delete localServer;
		localServer = nullptr;
	}
}

void LoopClient::connectToServer(std::string ip, unsigned int port, std::string userName, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (cmdArgs.gameState != NotInGame)
		leaveServer(cmdArgs);

	//TODO: Note without some kind of multithreading, this message will never display in the UI
	pd.serverBrowser->setConnectionNote("Connecting to server...");
	info("Attempting connection to " + ip + ":" + std::to_string(port));

	if (userName.length() > 0)
		pd.state->addString("network/username", userName);
	pd.state->addString("network/lastip", ip);
	pd.state->addInt("network/lastport", port, true, "", 1, 65535);
	pd.state->exportToFile(ClientProgramData::stateFilePath);

	cmdArgs.gameState = Connecting;

	//If we're hosting our own local server, it needs to service its ENet host while
	//we wait here for the handshake to complete - nothing else is ticking it right now.
	std::function<void()> pump = nullptr;
	if (localServer)
		pump = [this, &cmdArgs, settings]() { localServer->run(0.f, cmdArgs, settings); };

	client = new Client(ip, port, settings->getInt("network/packetholdtime"), pump);

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

	//How we look, which the server keeps until its Lua puts it on our player with client:applyAppearance
	client->send(makeAppearanceChoicePacket(AppearanceEditor::loadAppearance(settings)), JoinNegotiation);

	//From here, further initalization will actually take place in Networking/PacketsFromServer/AcceptConnection.cpp
	//Assuming the server lets us join, of course
}

void LoopClient::hostSinglePlayer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	if (cmdArgs.gameState != NotInGame)
		leaveServer(cmdArgs);

	info("Starting server");

	localServer = new LoopServer(cmdArgs, settings);
	if (!localServer->isValid())
	{
		pd.serverBrowser->setConnectionNote("Could not start local server, see error log.");
		delete localServer;
		localServer = nullptr;
		return;
	}

	std::string userName = pd.state->getString("network/username");
	if (userName.length() < 1)
		userName = "Player";

	connectToServer("127.0.0.1", DEFAULT_PORT, userName, cmdArgs, settings);
}

//Puts the ghost brick on whatever the camera is pointing at, if anything is in reach
static void spawnGhostFromCamera(ClientProgramData& pd, Simulation& simulation)
{
	if (!pd.physicsWorld || !simulation.camera)
		return;

	glm::vec3 start = simulation.camera->getPosition();
	glm::vec3 direction = simulation.camera->getDirection();
	btRigidBody* ignore = simulation.controlledDynamics.empty() ? nullptr : simulation.controlledDynamics[0]->body;
	btVector3 hitPosition, hitNormal;
	if (pd.physicsWorld->doRaycast(g2b3(start), g2b3(start + direction * 250.0f), ignore, hitPosition, hitNormal))
		pd.ghostBrick.spawnAt(b2g3(hitPosition), b2g3(hitNormal));
}

//0 to 1 around the flashlight's colors: fades from white into red, goes around every hue, and fades back into white
static glm::vec3 flashlightColor(float cycle)
{
	static constexpr float fadeFromWhite = 0.1f;

	float saturation = std::clamp(std::min(cycle, 1.0f - cycle) / fadeFromWhite, 0.0f, 1.0f);
	glm::vec3 hue = glm::clamp(glm::abs(glm::mod(cycle * 6.0f + glm::vec3(0, 4, 2), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f);
	return glm::mix(glm::vec3(1), hue, saturation);
}

void LoopClient::updateFlashlight(float deltaT)
{
	//Held longer than this, the key cycles the color instead of switching the flashlight on or off
	static constexpr float holdForColorMS = 350.0f;
	//Once around every color and back to white
	static constexpr float colorCycleMS = 8000.0f;
	//How often the color goes to the server while cycling
	static constexpr float colorSendMS = 100.0f;

	//Polled every frame so a press while it's disabled doesn't go off later, and so a tap too quick to be down on any frame still counts
	bool pressed = pd.input->pollCommand(Flashlight);

	//The server turned it off when it disabled it
	if (!simulation.flashlightEnabled || !client)
	{
		flashlightOn = false;
		flashlightHeldMS = 0;
		flashlightCycling = false;
		return;
	}

	bool switched = false;
	if (pd.input->isCommandKeydown(Flashlight))
	{
		flashlightHeldMS += deltaT;
		if (flashlightHeldMS >= holdForColorMS)
		{
			//Comes on to show the colors going by
			if (!flashlightOn)
			{
				flashlightOn = true;
				switched = true;
			}

			flashlightCycling = true;
			flashlightCycle = std::fmod(flashlightCycle + deltaT / colorCycleMS, 1.0f);
			flashlightColorUnsent = true;
		}
	}
	else
	{
		if ((flashlightHeldMS > 0 || pressed) && !flashlightCycling)
		{
			flashlightOn = !flashlightOn;
			switched = true;
		}

		flashlightHeldMS = 0;
		flashlightCycling = false;
	}

	flashlightSinceSentMS += deltaT;
	if (switched || (flashlightColorUnsent && (!flashlightCycling || flashlightSinceSentMS >= colorSendMS)))
	{
		client->send(makeFlashlightPacket(flashlightOn, flashlightColor(flashlightCycle)), OtherReliable);
		flashlightSinceSentMS = 0;
		flashlightColorUnsent = false;
	}
}

bool LoopClient::placeHeldLight(Light& light, glm::vec3& position, glm::vec3& direction)
{
	//How far along the beam past where it leaves the holder's collision box the light sits
	//The drawn arm and hand can reach past the collision box, so this has to clear them too
	static constexpr float handClearance = 0.45f;
	//Without a hand, how far past the side of the collision box, so the head doesn't shadow it
	static constexpr float headClearance = 0.3f;

	std::shared_ptr<Dynamic> holder = light.holder.lock();
	if (!holder || holder->getID() != light.getHolderID())
	{
		holder = simulation.dynamics ? simulation.dynamics->find(light.getHolderID()) : nullptr;
		light.holder = holder;
	}

	if (!holder)
		return false;

	std::shared_ptr<Model> model = holder->getType()->getModel();

	//Our own flashlight points exactly where we look, rather than gliding after the direction the server was last sent
	if (holder->clientControlled && simulation.camera->target.lock() == holder)
		direction = simulation.camera->getDirection();

	//From the right hand as it's drawn, swinging with it, which is kept up to date even while our own player is hidden in first person
	int hand = model->getMeshIdx("Right_Hand");
	if (hand != -1)
	{
		glm::vec3 handCenter = holder->getMeshCenter(hand);

		//The collision box as it's drawn, which the hand is inside
		btTransform bodyTransform = holder->body->getWorldTransform();
		btQuaternion bodyRotation = bodyTransform.getRotation();
		glm::vec3 origin = holder->renderedTransformInitialized ? holder->renderedPosition : b2g3(bodyTransform.getOrigin());
		glm::quat rotation = holder->renderedTransformInitialized ? holder->renderedRotation : glm::quat(bodyRotation.w(), bodyRotation.x(), bodyRotation.y(), bodyRotation.z());
		glm::quat toBox = glm::inverse(rotation);
		glm::vec3 halfExtents = model->getColHalfExtents();
		glm::vec3 start = toBox * (handCenter - (origin + rotation * model->getColOffset()));
		glm::vec3 along = toBox * direction;

		//Slid out of the box along the beam, so the holder's own body can't shadow it: just past the hand when it points away from them,
		//around the far side of them when it points across or behind them, for a body still turning to face where they look
		float exit = std::numeric_limits<float>::max();
		for (int axis = 0; axis < 3; axis++)
		{
			if (std::abs(along[axis]) > 0.00001f)
				exit = std::min(exit, ((along[axis] > 0 ? halfExtents[axis] : -halfExtents[axis]) - start[axis]) / along[axis]);
		}
		if (exit == std::numeric_limits<float>::max())
			exit = 0;

		position = handCenter + direction * (std::max(exit, 0.0f) + handClearance);
		return true;
	}

	glm::vec3 eyes;
	if (holder->clientControlled)
	{
		//Where the camera puts the eyes of a dynamic we move ourselves, so our own flashlight never trails behind the view
		eyes = b2g3(holder->body->getWorldTransform().getOrigin()) + holder->interpolator.getRotation() * model->getEyePosition();
	}
	else
	{
		glm::vec3 drawnAt = holder->renderedTransformInitialized ? holder->renderedPosition : b2g3(holder->getPosition());
		eyes = drawnAt + holder->renderedRotation * model->getEyePosition();
	}

	glm::vec3 halfExtents = model->getColHalfExtents();
	position = eyes + direction * (std::max(halfExtents.x, halfExtents.z) + headClearance);
	return true;
}

void LoopClient::handleInput(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	pd.input->keystates = SDL_GetKeyboardState(NULL);

	//Event loop, mostly just passing stuff to InputMap (in-game controls) and UserInterface (gui controls)
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		if (pd.gui->handleInput(e, pd.input))
		{
			pd.context->setMouseLock(false);
		}
		pd.input->handleInput(e);

		if (e.type == SDL_QUIT)
		{
			//Remember the window size we're closing at so next launch starts at the same size
			//instead of the fixed default, which is what left saved ImGui window positions
			//(and the server browser) partially off-screen after a resize.
			//Fullscreen always uses graphics/startresolution from the settings menu instead
			if (!settings->getBool("graphics/startfullscreen"))
			{
				glm::vec2 resolution = pd.context->getResolution();
				pd.state->addInt("window/width", (int)resolution.x, true, "", 1, 10000);
				pd.state->addInt("window/height", (int)resolution.y, true, "", 1, 10000);
				pd.state->exportToFile(ClientProgramData::stateFilePath);
			}

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
				createWaterTargets(settings);
			}
		}
		else if (e.type == SDL_MOUSEMOTION && pd.context->getMouseLocked())
		{
			simulation.camera->turn(-(float)e.motion.xrel, -(float)e.motion.yrel);
		}
		else if (e.type == SDL_KEYDOWN)
		{
			//This one is not handled through input map because input map can be suppressed
			//By having one or more guis open, which defeats the purpose of a quick gui close key
			if (e.key.keysym.sym == SDLK_ESCAPE)
			{
				//The appearance editor's color window and painting close before the editor itself
				if (pd.appearanceEditor->isOpen() && pd.appearanceEditor->handleEscape())
				{
				}
				else if (pd.gui->getOpenWindowCount() == 0)
				{
					pd.escapeMenu->open();
					pd.context->setMouseLock(false);
				}
				else
				{
					pd.gui->closeOneWindow();
					if (!pd.context->getMouseLocked() && !pd.gui->getOpenWindowCount() && cmdArgs.gameState != NotInGame)
						pd.context->setMouseLock(true);
				}
			}
		}
		else if (e.type == SDL_MOUSEWHEEL && pd.context->getMouseLocked() && !pd.gui->shouldUnlockMouse())
		{
			//Scrolls through hot bar slots while building, like the old game
			int amount = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -e.wheel.y : e.wheel.y;
			pd.brickHotbar->scroll(amount);
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN && simulation.camera && !pd.gui->shouldUnlockMouse() && cmdArgs.gameState == InGame && !pd.appearanceEditor->isOpen())
		{
			int mx, my;
			int mask = SDL_GetMouseState(&mx, &my);

			float x = (float)mx / pd.context->getResolution().x * 2 - 1;
			float y = (float)my / pd.context->getResolution().y * 2 - 1;

			glm::vec3 worldPos = simulation.camera->mouseCoordsToWorldSpace(glm::vec2(x, y));
			glm::vec3 dir = simulation.camera->getDirection();

			//Holding the wrench key, a left click wrenches the brick under the crosshair instead, until there's a wrench item
			if (e.button.button == SDL_BUTTON_LEFT && pd.input->isCommandKeydown(Wrench))
				client->send(makeWrenchRequestPacket(simulation.camera->getPosition(), dir), OtherReliable);
			else
			{
				ENetPacket *mouseClickPacket = makeMouseClickPacket(worldPos, dir, mask);
				client->send(mouseClickPacket, OtherReliable);

				//While building, a left click puts the ghost brick wherever the crosshair points
				if ((mask & SDL_BUTTON_LMASK) && pd.brickHotbar->isBuilding())
					spawnGhostFromCamera(pd, simulation);
			}
		}
	}

	//Interacting with gui, don't move around in-game
	pd.input->supressed = pd.gui->wantsSuppression();

	//Someone just applied setting changes
	if (pd.settingsMenu->pollForChanges())
	{
		//A newly picked resolution resizes a windowed game right away, which is then remembered as the window size on exit
		//Fullscreen picks it up on the next launch
		glm::ivec2 pickedResolution(settings->getInt("graphics/startresolutionx"), settings->getInt("graphics/startresolutiony"));
		if (pickedResolution != pd.appliedStartResolution)
		{
			pd.appliedStartResolution = pickedResolution;
			if (!settings->getBool("graphics/startfullscreen"))
				pd.context->resizeWindow(pickedResolution.x, pickedResolution.y);
		}

		Logger::setDebug(settings->getBool("logger/verbose"));
		simulation.camera->updateSettings(settings);
		pd.gui->updateSettings(settings);
		simulation.idealBufferSize = settings->getInt("network/snapshotbuffer");
		createWaterTargets(settings);
		createShadowTarget(settings);
		pd.audio->setVolumes(settings->getFloat("audio/mastervolume"), settings->getFloat("audio/musicvolume"));
		pd.audio->setEnvironmentOptions(settings->getInt("audio/reverbquality"), settings->getInt("audio/occlusionquality"));
		pd.acousticProbe.setQuality(settings->getInt("audio/reverbquality"), settings->getInt("audio/occlusionquality"));
		pd.audio->setVoiceVolume(settings->getFloat("audio/voicevolume"));
		pd.voice->setMicrophone(settings->getString("audio/microphone"), settings->getFloat("audio/microphonevolume"));
		if (simulation.brickDebris)
			simulation.brickDebris->setLifetime(settings->getFloat("graphics/brickdebrisseconds"));
		pd.particles->setMaxParticles(settings->getInt("graphics/maxparticles"));
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

	WrenchSubmission wrenchSubmission;
	if (pd.wrenchDialog->takeSubmission(wrenchSubmission) && client)
		client->send(makeWrenchSubmitPacket(wrenchSubmission.brickID, wrenchSubmission.collides, wrenchSubmission.name, wrenchSubmission.attachments), OtherReliable);

	//Applied or closed, back to playing if nothing else is open
	if (wrenchDialogWasOpen && !pd.wrenchDialog->isOpen() && pd.gui->getOpenWindowCount() == 0 && cmdArgs.gameState == InGame)
		pd.context->setMouseLock(true);
	wrenchDialogWasOpen = pd.wrenchDialog->isOpen();

	if (pd.serverBrowser->serverPickReady())
	{
		std::string ip,userName;
		int port;
		pd.serverBrowser->getServerData(ip, port,userName);
		connectToServer(ip, port,userName,cmdArgs,settings);
	}

	if (pd.serverBrowser->singlePlayerReady())
	{
		pd.serverBrowser->clearSinglePlayerReady();
		hostSinglePlayer(cmdArgs, settings);
	}

	if (pd.serverBrowser->settingsReady())
	{
		pd.serverBrowser->clearSettingsReady();
		pd.settingsMenu->open();
	}

	if (pd.serverBrowser->appearanceReady())
	{
		pd.serverBrowser->clearAppearanceReady();
		pd.serverBrowser->close();
		pd.appearanceEditor->open();
	}

	//Saved or not
	if (appearanceEditorWasOpen && !pd.appearanceEditor->isOpen())
		pd.serverBrowser->open();
	appearanceEditorWasOpen = pd.appearanceEditor->isOpen();

	//Saving while connected changes our player right away, if the server's Lua put our appearance on it
	if (pd.appearanceEditor->takeSaved() && client)
		client->send(makeAppearanceChoicePacket(AppearanceEditor::loadAppearance(settings)), OtherReliable);

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

		case JoinServer:
		{
			pd.serverBrowser->open();
			break;
		}

		case None:
		default:
			break;
	}

	if(cmdArgs.gameState == NotInGame)
	{
		return;
	}

	//Various keys were pressed that were bound to certain commands:
	if (pd.input->pollCommand(MouseLock))
		pd.context->setMouseLock(!pd.context->getMouseLocked());

	//Move camera around
	simulation.camera->control(deltaT, pd.input);

	//Narrows the view while held, like the old game
	simulation.camera->zooming = pd.input->isCommandKeydown(Zoom);

	if (cmdArgs.gameState == InGame)
		updateFlashlight(deltaT);

	if (pd.input->pollCommand(FirstThirdPerson))
		simulation.camera->swapPerson();

	if (pd.input->pollCommand(DebugView))
		pd.debugMenu->showDebugPhysicsView = !pd.debugMenu->showDebugPhysicsView;

	//Building
	if (pd.input->pollCommand(OpenBrickSelector))
	{
		pd.brickSelector->open();
		pd.context->setMouseLock(false);
	}

	HotbarBrick picked;
	bool slotsChanged = false;
	while (pd.brickSelector->popPick(picked))
	{
		pd.brickHotbar->add(picked);
		slotsChanged = true;
	}

	bool colorChanged = pd.brickSelector->takeColorChanged();
	if (slotsChanged || colorChanged)
	{
		pd.brickHotbar->save(pd.state);
		pd.brickSelector->save(pd.state);

		//Written right away rather than on exit, since not every way of quitting saves
		pd.state->exportToFile(ClientProgramData::stateFilePath);
	}
	pd.brickHotbar->peek = pd.brickSelector->isOpen();

	for (int a = 0; a < BrickHotbar::slotCount; a++)
	{
		if (pd.input->pollCommand((InputCommand)(UseBrick1 + a)))
			pd.brickHotbar->pressSlot(a);
	}
	if (pd.input->pollCommand(HideGhostBrick))
		pd.brickHotbar->putAway();

	if (pd.brickHotbar->takeChange())
	{
		HotbarBrick building;
		if (pd.brickHotbar->getSelected(building))
		{
			//Special slots hold a type name, which might not be among the types this copy of the game has
			int special = building.special ? pd.brickTypes.findSpecial(building.name) : -1;
			const SpecialBrickType* type = pd.brickTypes.getSpecial(special);

			if (building.special && !type)
			{
				pd.ghostBrick.hide();
				pd.gui->addCenterPrint("Missing special brick " + building.name, 2000, 1.0f, 0.4f, 0.4f);
			}
			else
			{
				//Starting to build again begins in move mode, like the old game
				if (!pd.ghostBrick.isVisible())
					pd.ghostBrick.setResizeMode(false);

				if (type)
					pd.ghostBrick.select(type->width, type->height, type->length, (uint16_t)(special + 1));
				else
					pd.ghostBrick.select(building.width, building.height, building.length);

				if (!pd.ghostBrick.show())
					spawnGhostFromCamera(pd, simulation);
			}
		}
		else
			pd.ghostBrick.hide();
	}

	pd.ghostBrick.setColor(pd.brickSelector->getColor());
	pd.ghostBrick.setMaterial(pd.brickSelector->getMaterial());
	pd.ghostBrick.update(deltaT, pd.input, simulation.camera->getDirection());

	//Clicks like the old game, if the server registered sounds by these names
	if (pd.ghostBrick.didRotate())
		pd.audio->playSound("ClickRotate");
	if (pd.ghostBrick.didMove())
		pd.audio->playSound("ClickMove");

	//Polled every frame so presses made while the ghost is hidden don't fire later
	if (pd.input->pollCommand(PlantBrick) && pd.ghostBrick.isVisible())
	{
		//Special types go by the server's ID for them
		Brick planted = pd.ghostBrick.get();
		if (planted.isSpecial())
			planted.typeID = planted.typeID < simulation.brickTypeToServer.size() ? simulation.brickTypeToServer[planted.typeID] : 0;

		if (pd.ghostBrick.get().isSpecial() && !planted.isSpecial())
			pd.gui->addCenterPrint("This server doesn't have that brick", 2000, 1.0f, 0.4f, 0.4f);
		else
			client->send(makePlantBrickPacket(planted), OtherReliable);
	}

	//The key alone does nothing, undo is Ctrl plus the bound key
	//A press removes one brick, holding it keeps removing more after a short delay
	static constexpr float undoRepeatDelayMS = 500.0f;
	static constexpr float undoRepeatIntervalMS = 100.0f;

	bool ctrlDown = SDL_GetModState() & KMOD_CTRL;
	bool undoPressed = pd.input->pollCommand(UndoBrick) && ctrlDown;
	bool undoHeld = ctrlDown && pd.input->isCommandKeydown(UndoBrick);

	if (undoPressed)
	{
		client->send(makeUndoBrickPacket(), OtherReliable);
		undoHeldMS = 0;
		undoSinceRepeatMS = 0;
	}
	else if (undoHeld)
	{
		undoHeldMS += deltaT;
		undoSinceRepeatMS += deltaT;
		if (undoHeldMS > undoRepeatDelayMS && undoSinceRepeatMS > undoRepeatIntervalMS)
		{
			client->send(makeUndoBrickPacket(), OtherReliable);
			undoSinceRepeatMS = 0;
		}
	}
	else
		undoHeldMS = 0;
}

void LoopClient::predictLocalCollisions()
{
	if (!pd.physicsWorld)
		return;

	//How long a predicted local reaction is trusted before falling back to the (by-then-hopefully-arrived) server state
	const unsigned int predictionWindowMS = 250;

	//A pushed object almost always outruns the player's own walk speed within a frame or two, so contact - and with it,
	//the window above - ends almost immediately even though the object is still clearly sliding from the push. Without
	//this, the server's still-stale (pre-push) position would stomp it mid-slide, and since the player keeps walking
	//into it, that repeats continuously and looks like rubber-banding rather than a single correction
	const float stillMovingSpeedThreshold = 0.5f;

	//Upper bound on how long continued motion can keep extending prediction *after we've lost contact* with the
	//object (e.g. it slides/falls on past the point where we last touched it). Client and server resolve a many-body
	//pileup independently and can diverge into totally unrelated resting positions the longer prediction runs
	//unmoored from an actual confirmed touch, so we want to cap that divergence risk - but this must NOT count time
	//spent still actively touching the object (see predictLocallyStartedAt below), or a long sustained push (e.g.
	//walking a cube toward a ledge) gets cut off mid-fall once it goes over, handing back to a server position that
	//hasn't caught up yet and causing a visible correction right in the middle of an otherwise-normal interaction
	const unsigned int maxPredictionDurationMS = 1500;

	//True if this object is touching some OTHER (non-player) dynamic right now - not safe to predict, since that's
	//exactly where tiny numeric/ordering differences between the client's and server's independently-run simulations
	//get amplified into completely different outcomes. A lone object interacting only with the player and static
	//geometry (ground, bricks - identical on both sides since they don't move) stays close enough to the server's
	//own version to predict safely; a multi-body pileup does not. See the brainstorm on why piles teleport on settle
	auto touchingOtherDynamics = [&](const std::shared_ptr<Dynamic>& obj) -> bool
	{
		for (btRigidBody* other : pd.physicsWorld->getTouching(obj->body))
		{
			if (other->getUserIndex() != dynamicBody)
				continue;

			std::shared_ptr<Dynamic> otherDynamic = dynamicFromBody(other);
			if (otherDynamic && !otherDynamic->clientControlled)
				return true;
		}
		return false;
	};

	for (unsigned int i = 0; i < simulation.controlledDynamics.size(); i++)
	{
		std::shared_ptr<Dynamic> controlled = simulation.controlledDynamics[i];
		if (!controlled || !controlled->body)
			continue;

		for (btRigidBody* other : pd.physicsWorld->getTouching(controlled->body))
		{
			if (other->getUserIndex() != dynamicBody)
				continue;

			std::shared_ptr<Dynamic> touched = dynamicFromBody(other);
			if (touched && !touched->clientControlled && !touchingOtherDynamics(touched))
			{
				//Refreshed every frame we're actually touching it, so continuous pushing never runs into the cap
				//below - it only starts counting once contact actually ends, which is the point it's meant to bound
				touched->predictLocallyStartedAt = getTicksMS();
				touched->predictLocallyUntil = getTicksMS() + predictionWindowMS;
			}
		}
	}

	if (simulation.dynamics)
	{
		for (unsigned int i = 0; i < simulation.dynamics->size(); i++)
		{
			std::shared_ptr<Dynamic> d = simulation.dynamics->get(i);
			if (!d || !d->body || d->clientControlled)
				continue;

			bool stillPredicting = getTicksMS() < d->predictLocallyUntil;
			bool stillMovingFast = d->body->getLinearVelocity().length2() > stillMovingSpeedThreshold * stillMovingSpeedThreshold;
			bool underEpisodeCap = getTicksMS() < d->predictLocallyStartedAt + maxPredictionDurationMS;

			//Stop extending (letting the existing window run out naturally) the moment it touches another dynamic -
			//e.g. a cube pushed off a ledge lands on top of a different cube mid-fall
			if (stillPredicting && stillMovingFast && underEpisodeCap && !touchingOtherDynamics(d))
				d->predictLocallyUntil = getTicksMS() + predictionWindowMS;
		}
	}
}

void LoopClient::createWaterTargets(std::shared_ptr<SettingManager> settings)
{
	pd.waterReflection.reset();
	pd.waterRefraction.reset();

	//0 = off, 1 = half resolution, 2 = full resolution
	int quality = settings->getInt("graphics/waterquality");
	if (quality <= 0)
		return;

	int divisor = quality == 1 ? 2 : 1;

	RenderTarget::RenderTargetSettings waterSettings;
	waterSettings.width = std::max(1, (int)pd.context->getResolution().x / divisor);
	waterSettings.height = std::max(1, (int)pd.context->getResolution().y / divisor);

	pd.waterReflection = std::make_shared<RenderTarget>(waterSettings, pd.textures);
	pd.waterRefraction = std::make_shared<RenderTarget>(waterSettings, pd.textures);
}

void LoopClient::createShadowTarget(std::shared_ptr<SettingManager> settings)
{
	pd.shadowSoftness = std::min(std::max(settings->getInt("graphics/shadowsoftness"), 0), 3);

	//0 = 2k, 1 = 4k, 2 = 8k, capped at what the graphics card allows
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	int resolution = 2048 << std::min(std::max(settings->getInt("graphics/shadowresolution"), 0), 2);
	if (maxTextureSize > 0)
		resolution = std::min(resolution, (int)maxTextureSize);

	if (!pd.shadows || resolution != pd.shadowResolution)
	{
		pd.shadowResolution = resolution;

		RenderTarget::RenderTargetSettings shadowSettings;
		shadowSettings.width = resolution;
		shadowSettings.height = resolution;
		shadowSettings.layers = 3;
		shadowSettings.useColor = false;
		shadowSettings.depthCompare = true;
		pd.shadows.reset();
		pd.shadows = std::make_shared<RenderTarget>(shadowSettings, pd.textures);
	}

	pd.coloredShadows = settings->getBool("graphics/shadowcolor");

	//Each cube face is a quarter of a cascade, capped so 8 shadowed lights stay under 200 MB, plus under 100 MB of tint maps with colored shadows
	pd.pointLights->setShadowSettings(settings->getInt("graphics/pointshadows"), std::min(resolution / 4, 1024), pd.coloredShadows, pd.textures);

	//Half resolution to save memory, colored shadows just come out a little softer
	int tintResolution = pd.coloredShadows ? std::max(1, resolution / 2) : 1;
	if (pd.shadowTint && tintResolution == pd.shadowTintResolution)
		return;
	pd.shadowTintResolution = tintResolution;

	RenderTarget::RenderTargetSettings tintSettings;
	tintSettings.width = tintResolution;
	tintSettings.height = tintResolution;
	tintSettings.layers = 3;
	tintSettings.channels = 3;
	tintSettings.depthCompare = true;
	//Untinted light, which each transparent brick then multiplies its color into
	tintSettings.clearColor = glm::vec4(1);
	pd.shadowTint.reset();
	pd.shadowTint = std::make_shared<RenderTarget>(tintSettings, pd.textures);
}

void LoopClient::renderScene(bool clipAtWater)
{
	//Sky is behind everything, so it's drawn first without touching depth
	pd.shaders->skyShader->use();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBindVertexArray(pd.skyVao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	//The sky shader doesn't write gl_ClipDistance, so clipping can only be turned on after it
	if (clipAtWater)
		glEnable(GL_CLIP_DISTANCE0);

	pd.shaders->modelShader->use();
	glUniformMatrix4fv(pd.lightSpaceMatriciesUniformModel, 3, GL_FALSE, (GLfloat*)pd.lightSpaceMatricies);
	glUniform1i(pd.shaders->modelShader->getUniformLocation("shadowSoftness"), pd.shadowSoftness);
	glUniform1i(pd.shaders->modelShader->getUniformLocation("coloredShadows"), pd.tintShadowsActive);
	pd.shadows->bindDepthResult(ShadowArray);
	pd.shadowTint->bindDepthResult(TintDepthArray);
	pd.shadowTint->bindColorResult(TintColorArray);
	pd.pointLights->bindShadowMaps();

	//Models:
	pd.shaders->basicUniforms.nonInstanced = 0;
	pd.shaders->basicUniforms.cameraSpacePosition = 0;
	pd.shaders->updateBasicUBO();
	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->render(pd.shaders);

	//Render grass
	pd.shaders->basicUniforms.ScaleMatrix = glm::mat4(1.0);
	pd.shaders->basicUniforms.TranslationMatrix = glm::mat4(1.0);
	pd.shaders->basicUniforms.RotationMatrix = glm::mat4(1.0);
	pd.shaders->basicUniforms.nonInstanced = 1;
	pd.shaders->basicUniforms.cameraSpacePosition = 1;
	pd.shaders->updateBasicUBO();
	pd.grassMaterial->use(pd.shaders);
	glBindVertexArray(pd.grassVao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Bricks, transparent ones last
	pd.shaders->brickShader->use();
	glUniformMatrix4fv(pd.lightSpaceMatriciesUniformBrick, 3, GL_FALSE, (GLfloat*)pd.lightSpaceMatricies);
	glUniform1i(pd.shaders->brickShader->getUniformLocation("shadowSoftness"), pd.shadowSoftness);
	glUniform1i(pd.shaders->brickShader->getUniformLocation("coloredShadows"), pd.tintShadowsActive);
	pd.brickRenderer->render(pd.shaders, false);

	//Only in the main view, not reflected or refracted by water
	//Debris writes depth, so it goes before anything drawn without depth writes
	if (!clipAtWater && simulation.brickDebris)
		simulation.brickDebris->render(pd.shaders, pd.brickRenderer);

	if (clipAtWater)
		glDisable(GL_CLIP_DISTANCE0);
}

void LoopClient::renderTransparent(bool clipAtWater)
{
	if (clipAtWater)
		glEnable(GL_CLIP_DISTANCE0);

	pd.shaders->brickShader->use();
	glUniformMatrix4fv(pd.lightSpaceMatriciesUniformBrick, 3, GL_FALSE, (GLfloat*)pd.lightSpaceMatricies);
	glUniform1i(pd.shaders->brickShader->getUniformLocation("shadowSoftness"), pd.shadowSoftness);
	glUniform1i(pd.shaders->brickShader->getUniformLocation("coloredShadows"), pd.tintShadowsActive);
	pd.shadows->bindDepthResult(ShadowArray);
	pd.shadowTint->bindDepthResult(TintDepthArray);
	pd.shadowTint->bindColorResult(TintColorArray);
	pd.pointLights->bindShadowMaps();
	pd.brickRenderer->render(pd.shaders, true);

	if (!clipAtWater && pd.ghostBrick.isVisible())
	{
		//Pulses a bit under once a second so the ghost can't be mistaken for a planted transparent brick
		float pulse = 0.5f + 0.5f * std::sin(SDL_GetTicks() / 1000.0f * 6.2831853f * 0.8f);
		pd.brickRenderer->renderGhost(pd.shaders, pd.ghostBrick.get(), pulse);
	}

	//Neither writes depth, so they go after everything that does
	pd.particles->render(pd.shaders, pd.lightSpaceMatricies);
	pd.pointLights->renderCoronae(pd.shaders);

	if (clipAtWater)
		glDisable(GL_CLIP_DISTANCE0);
}

void LoopClient::makeWaterRipples(float deltaT)
{
	//Vertical speed, world units per second, below which going in or out of the water doesn't ripple
	static constexpr float crossingSpeed = 1.0f;
	//Slower than this, something at the surface leaves no wake, so floating things bobbing in place stay quiet
	static constexpr float wakeSpeed = 1.0f;

	pd.waterRipples.update(deltaT / 1000.0f);

	if (!simulation.dynamics)
		return;

	for (unsigned int a = 0; a < simulation.dynamics->size(); a++)
	{
		std::shared_ptr<Dynamic> d = simulation.dynamics->get(a);

		if (!simulation.waterEnabled || !d->renderedTransformInitialized)
		{
			d->rippleStateKnown = false;
			continue;
		}

		//Around where it's drawn, the body of someone else's dynamic can be off between server updates
		btVector3 aabbMin, aabbMax;
		d->body->getAabb(aabbMin, aabbMax);
		glm::vec3 halfSize = b2g3(aabbMax - aabbMin) * 0.5f;
		glm::vec3 center = d->renderedPosition + d->renderedRotation * d->getType()->getModel()->getColOffset();
		float bottom = center.y - halfSize.y;
		float top = center.y + halfSize.y;

		//Same gap between going in and coming out as LoopServer::playWaterSounds
		bool wasInWater = d->rippleInWater;
		if (!wasInWater && bottom < simulation.waterLevel)
			d->rippleInWater = true;
		else if (wasInWater && bottom > simulation.waterLevel + 0.5f)
			d->rippleInWater = false;

		//Just showed up, or water just appeared under it
		if (!d->rippleStateKnown)
		{
			d->rippleStateKnown = true;
			d->rippleWakeDistance = 0;
			continue;
		}

		glm::vec2 surface(center.x, center.z);
		float size = std::max(halfSize.x, halfSize.z);
		//How it's actually moving, see Dynamic::updateSnapshot
		const glm::vec3& velocity = d->tiltVelocity;

		if (d->rippleInWater != wasInWater)
		{
			if (std::abs(velocity.y) > crossingSpeed)
				pd.waterRipples.add(surface, std::clamp(std::abs(velocity.y) / 15.0f, 0.2f, 1.0f) * (d->rippleInWater ? 1.0f : 0.5f), size);
			d->rippleWakeDistance = 0;
		}
		else if (d->rippleInWater && top > simulation.waterLevel)
		{
			//Wading, swimming, or floating along: small short lived ripples every so often
			float speed = glm::length(velocity);
			if (speed > wakeSpeed)
				d->rippleWakeDistance += speed * deltaT / 1000.0f;

			if (d->rippleWakeDistance > std::max(2.0f, size * 1.5f))
			{
				pd.waterRipples.add(surface, std::clamp(speed / 30.0f, 0.1f, 0.35f), size, 1.2f);
				d->rippleWakeDistance = 0;
			}
		}
	}
}

void LoopClient::updateParticles()
{
	//Particles don't get far from their emitters, so emitters well past the end of the fog don't eject any
	static constexpr float ejectPastFog = 64.0f;

	double nowMS = ParticleSystem::getNowMS();
	glm::vec3 cameraPosition = simulation.camera->getPosition();
	float ejectDistance = pd.environment.fogDistanceMax + ejectPastFog;

	if (simulation.emitters)
	{
		int64_t ticks = SDL_GetTicks();

		for (unsigned int a = 0; a < simulation.emitters->size(); a++)
		{
			std::shared_ptr<Emitter> emitter = simulation.emitters->get(a);
			glm::vec3 position = emitter->getPosition();
			glm::quat rotation = glm::quat(1, 0, 0, 0);
			glm::vec3 velocity = glm::vec3(0);

			if (emitter->getAttachKind() == EmitterAttachDynamic)
			{
				std::shared_ptr<Dynamic> target = emitter->dynamic.lock();
				if (!target || target->getID() != emitter->getDynamicID())
				{
					target = simulation.dynamics ? simulation.dynamics->find(emitter->getDynamicID()) : nullptr;
					emitter->dynamic = target;
				}

				//Hasn't arrived yet, or is gone and the emitter's removal is on its way
				if (!target)
				{
					ParticleSystem::skipEmission(emitter->clock, position, rotation, nowMS);
					continue;
				}

				position = target->getMeshCenter(emitter->getMeshIndex());
				rotation = target->getMeshRotation(emitter->getMeshIndex());
				velocity = b2g3(target->getVelocity());
			}

			//The server removes it too, this just keeps a short burst from running long while that's on its way
			const EmitterTypeData* type = pd.particles->getEmitterType(emitter->getTypeID());
			bool expired = type && emitter->getAttachKind() != EmitterAttachBrick && type->lifetimeMS > 0 && ticks - emitter->startMS > (int64_t)type->lifetimeMS;

			if (expired || glm::distance(position, cameraPosition) > ejectDistance)
				ParticleSystem::skipEmission(emitter->clock, position, rotation, nowMS);
			else
				pd.particles->emit(emitter->clock, emitter->getTypeID(), position, rotation, velocity, nowMS);
		}
	}

	pd.particles->update(nowMS, cameraPosition, pd.environment.fogDistanceMax);
}

void LoopClient::renderEverything(float deltaT)
{
	//TODO: Get rid of this
	if (simulation.dynamics)
	{
		for (unsigned a = 0; a < simulation.dynamics->size(); a++)
		{
			std::shared_ptr<Dynamic> d = simulation.dynamics->get(a);
			bool predictingLocally = getTicksMS() < d->predictLocallyUntil;

			if (d->wasPredictingLocally && !predictingLocally)
				d->handOffFromPrediction(simulation.idealBufferSize);
			d->wasPredictingLocally = predictingLocally;

			d->updateSnapshot(deltaT, pd.debugMenu->showDebugPhysicsView || predictingLocally, simulation.waterEnabled ? simulation.waterLevel : PlayerController::noWater);
		}
	}

	makeWaterRipples(deltaT);

	//Technically rendering related calculations based on previously inputted transform data
	for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
		simulation.dynamicTypes[a]->getModel()->updateAll(deltaT);

	simulation.camera->render(pd.shaders, deltaT, pd.physicsWorld);

	pd.environment.cycle = simulation.dayCycle;
	pd.environment.calc(simulation.worldTimeSeconds);
	pd.environment.passUniforms(pd.shaders);
	//Every wave in water.vert/frag completes a whole number of cycles per 100 seconds, so wrapping here is seamless
	pd.shaders->environmentUniforms.WaveTime = (float)fmod(getTicksMS() / 1000.0, 100.0);
	pd.shaders->environmentUniforms.WaterLevel = simulation.waterLevel;
	pd.shaders->environmentUniforms.HorizonHeight = simulation.waterEnabled ? std::max(0.0f, simulation.waterLevel) : 0.0f;
	pd.shaders->environmentUniforms.ClipPlane = glm::vec4(0);
	pd.shaders->updateEnvironmentUBO();

	updateParticles();

	//Fog fully hides anything past fogDistanceMax, so shadows don't need to reach further
	simulation.camera->calculateLightSpaceMatricies(pd.environment.lightDirection, pd.environment.fogDistanceMax, pd.shadowResolution, pd.lightSpaceMatricies);

	pd.brickRenderer->rebuildDirty(4.0f);

	//Render shadows to texture, one cascade at a time so each only draws the chunks that can cast into it
	//Casters between the light and a cascade still shadow it, flattened onto its near plane instead of clipped away
	glEnable(GL_DEPTH_CLAMP);
	//Pushes casters back by their slope, more for wider filters, so a filter's outer samples don't land on the surface being shaded
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f + (float)pd.shadowSoftness, 1.0f);

	pd.shaders->basicUniforms.nonInstanced = 0;
	pd.shaders->basicUniforms.cameraSpacePosition = 0;
	pd.shaders->updateBasicUBO();

	//With colored shadows on, transparent bricks tint the light passing through them instead of blocking it
	pd.tintShadowsActive = pd.coloredShadows && pd.brickRenderer->hasTransparentBricks();

	//A point light's shadows leave out the bricks it's inside, so a light in the middle of its brick shines out of it. The sun's leave out none
	auto setSkippedPoint = [](GLint containingUniform, GLint pointUniform, const glm::vec3* lightPosition)
	{
		glUniform1i(containingUniform, lightPosition ? 1 : 0);
		if (lightPosition)
			glUniform3fv(pointUniform, 1, &(*lightPosition)[0]);
	};

	//Into the bound layer of a tint map, for the sun's cascades (no light position) and point lights' cube faces alike
	auto drawShadowTint = [this, setSkippedPoint](const glm::mat4& lightSpaceMatrix, const glm::vec3* lightPosition)
	{
		glCullFace(GL_FRONT);

		//Depth of the transparent brick nearest the light, so surfaces in front of it aren't tinted
		//Every transparent brick tints however see-through it is, the tint just gets fainter
		pd.shaders->brickShadowCascadeShader->use();
		glUniformMatrix4fv(pd.shadowCascadeMatrixUniformBrick, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
		glUniform1f(pd.shadowCascadeMinOpacityUniform, 0.0f);
		setSkippedPoint(pd.shadowCascadeSkipContainingUniform, pd.shadowCascadeSkipPointUniform, lightPosition);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		pd.brickRenderer->renderShadowCascade(lightSpaceMatrix, false, true);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		//Each channel keeps the least light any transparent brick along the way lets through, so order doesn't matter and
		//a wall of stacked transparent plates tints like one brick instead of compounding into a plain shadow
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendEquation(GL_MIN);
		pd.shaders->brickShadowTintShader->use();
		glUniformMatrix4fv(pd.shadowTintMatrixUniform, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
		glUniform1f(pd.shadowTintMinOpacityUniform, 0.0f);
		setSkippedPoint(pd.shadowTintSkipContainingUniform, pd.shadowTintSkipPointUniform, lightPosition);
		pd.brickRenderer->renderShadowCascade(lightSpaceMatrix, false, true, true);
		glBlendEquation(GL_FUNC_ADD);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);

		glCullFace(GL_BACK);
	};

	for (int cascade = 0; cascade < 3; cascade++)
	{
		pd.shadows->useLayer(cascade);

		//Models aren't guaranteed to be closed meshes, so both sides cast
		glDisable(GL_CULL_FACE);
		pd.shaders->modelShadowCascadeShader->use();
		glUniformMatrix4fv(pd.shadowCascadeMatrixUniformModel, 1, GL_FALSE, &pd.lightSpaceMatricies[cascade][0][0]);
		for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
			simulation.dynamicTypes[a]->render(pd.shaders, false);
		glEnable(GL_CULL_FACE);

		//Bricks are closed boxes, so only their far sides are drawn, which leaves a whole brick between a lit face and the depth it's compared to
		glCullFace(GL_FRONT);
		pd.shaders->brickShadowCascadeShader->use();
		glUniformMatrix4fv(pd.shadowCascadeMatrixUniformBrick, 1, GL_FALSE, &pd.lightSpaceMatricies[cascade][0][0]);
		//Without colored shadows, mostly see-through bricks don't block any light
		glUniform1f(pd.shadowCascadeMinOpacityUniform, 0.5f);
		setSkippedPoint(pd.shadowCascadeSkipContainingUniform, pd.shadowCascadeSkipPointUniform, nullptr);
		pd.brickRenderer->renderShadowCascade(pd.lightSpaceMatricies[cascade], true, !pd.tintShadowsActive);

		if (pd.tintShadowsActive)
		{
			pd.shadowTint->useLayer(cascade);
			drawShadowTint(pd.lightSpaceMatricies[cascade], nullptr);
		}

		glCullFace(GL_BACK);
	}

	//Point light shadows use perspective views that already start right at the light
	glDisable(GL_DEPTH_CLAMP);

	std::vector<PointLightSource> lightSources;
	if (simulation.lights)
	{
		Uint32 now = SDL_GetTicks();
		lightSources.reserve(simulation.lights->size());
		for (unsigned int a = 0; a < simulation.lights->size(); a++)
		{
			std::shared_ptr<Light> light = simulation.lights->get(a);
			glm::vec3 position = light->getRenderedPosition(now);
			glm::vec3 direction = light->getRenderedDirection(now);

			//Flashlights, skipped until the player holding one arrives
			if (light->getHolderID() != NO_ID && !placeHeldLight(*light, position, direction))
				continue;

			lightSources.push_back({ light->getID(), position, light->getColor(), light->getBrightness(), light->getCoronaWidth(), light->getRange(),
				direction, light->getConeCosine() });
		}
	}

	const CameraUniforms& view = pd.shaders->cameraUniforms;
	pd.pointLights->update(lightSources, pd.shaders, view.CameraPosition, view.CameraProjection * view.CameraView, pd.environment.fogDistanceMax);

	//Dynamics can move or animate at any time, so a light with one in range redraws its shadows every frame
	auto movingCastersNear = [this](const glm::vec3& position, float range) -> bool
	{
		if (!simulation.dynamics)
			return false;

		for (unsigned int a = 0; a < simulation.dynamics->size(); a++)
		{
			btVector3 aabbMin, aabbMax;
			simulation.dynamics->get(a)->body->getAabb(aabbMin, aabbMax);
			glm::vec3 closest = glm::clamp(position, b2g3(aabbMin), b2g3(aabbMax));
			if (glm::dot(closest - position, closest - position) < range * range)
				return true;
		}
		return false;
	};

	auto drawPointShadowCasters = [this, setSkippedPoint](const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPosition, bool tinted)
	{
		glDisable(GL_CULL_FACE);
		pd.shaders->modelShadowCascadeShader->use();
		glUniformMatrix4fv(pd.shadowCascadeMatrixUniformModel, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
		for (unsigned int a = 0; a < simulation.dynamicTypes.size(); a++)
			simulation.dynamicTypes[a]->render(pd.shaders, false);
		glEnable(GL_CULL_FACE);

		//Like the sun: transparent bricks tint the light instead of blocking it, or without colored shadows the ones at least half opaque block it
		glCullFace(GL_FRONT);
		pd.shaders->brickShadowCascadeShader->use();
		glUniformMatrix4fv(pd.shadowCascadeMatrixUniformBrick, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
		glUniform1f(pd.shadowCascadeMinOpacityUniform, 0.5f);
		setSkippedPoint(pd.shadowCascadeSkipContainingUniform, pd.shadowCascadeSkipPointUniform, &lightPosition);
		pd.brickRenderer->renderShadowCascade(lightSpaceMatrix, true, !tinted);
		glCullFace(GL_BACK);
	};

	auto drawPointShadowTint = [drawShadowTint](const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPosition)
	{
		drawShadowTint(lightSpaceMatrix, &lightPosition);
	};

	pd.pointLights->renderShadows(pd.brickRenderer->getGeneration() + simulation.staticsChanged, pd.tintShadowsActive, movingCastersNear, drawPointShadowCasters, drawPointShadowTint);

	glDisable(GL_POLYGON_OFFSET_FILL);

	bool cameraUnderwater = simulation.camera->getPosition().y < simulation.waterLevel;
	bool renderWaterPasses = simulation.waterEnabled && pd.waterReflection && pd.waterRefraction;

	auto setClipPlane = [this](const glm::vec4& plane)
	{
		pd.shaders->environmentUniforms.ClipPlane = plane;
		pd.shaders->updateEnvironmentUBO();
	};

	//Keep a little past the surface so wave troughs don't open gaps where objects meet the water
	const float clipOverlap = 0.25f;

	if (renderWaterPasses)
	{
		//Reflection: the scene above the water, from a camera mirrored below the surface
		if (!cameraUnderwater)
		{
			pd.waterReflection->use();
			simulation.camera->uploadReflectionUniforms(pd.shaders, simulation.waterLevel);
			setClipPlane(glm::vec4(0, 1, 0, clipOverlap - simulation.waterLevel));
			renderScene(true);
			renderTransparent(true);
			simulation.camera->uploadUniforms(pd.shaders);
		}

		//Refraction: whatever is on the other side of the surface from the camera
		pd.waterRefraction->use();
		if (cameraUnderwater)
			setClipPlane(glm::vec4(0, 1, 0, clipOverlap - simulation.waterLevel));
		else
			setClipPlane(glm::vec4(0, -1, 0, clipOverlap + simulation.waterLevel));
		renderScene(true);
		renderTransparent(true);

		setClipPlane(glm::vec4(0));
	}

	//Start rendering to screen:
	pd.context->select();
	pd.context->clear(pd.environment.fogColor.r, pd.environment.fogColor.g, pd.environment.fogColor.b);
	renderScene(false);

	if (simulation.waterEnabled)
	{
		pd.shaders->waterShader->use();
		pd.pointLights->bindShadowMaps();
		//Always reaches past the end of the fog, so its edge is never visible
		float surfaceRadius = std::max(waterRadius, pd.environment.fogDistanceMax + 10.0f);
		glUniform1f(pd.shaders->waterShader->getUniformLocation("waterRadius"), surfaceRadius);
		glUniform1f(pd.shaders->waterShader->getUniformLocation("gridSpacing"), surfaceRadius * 2.0f / waterGridCells);
		glUniform1i(pd.shaders->waterShader->getUniformLocation("useReflection"), renderWaterPasses && !cameraUnderwater);
		glUniform1i(pd.shaders->waterShader->getUniformLocation("useRefraction"), renderWaterPasses);
		glUniform1i(pd.shaders->waterShader->getUniformLocation("cameraUnderwater"), cameraUnderwater);
		pd.waterRipples.upload(pd.shaders->waterShader, simulation.camera->getPosition());

		if (renderWaterPasses)
		{
			pd.waterReflection->bindColorResult(Reflection);
			pd.waterRefraction->bindColorResult(Refraction);
		}

		//Visible from both above and below
		glDisable(GL_CULL_FACE);
		glBindVertexArray(pd.waterVao);
		glDrawArrays(GL_TRIANGLES, 0, pd.waterVertexCount);
		glBindVertexArray(0);
		glEnable(GL_CULL_FACE);
	}

	//After the water, which writes depth, so water behind a transparent brick can't paint over it
	renderTransparent(false);

	//Outlines/highlights: a selection-style indicator that should show through everything in the scene except
	//its own source object (so it doesn't just paint a solid blob over the object it's highlighting) and other
	//highlights (so overlapping highlights never fight over which one is "in front"). Scene depth is ignored
	//entirely (X-ray), and a per-object stencil mask excludes exactly that object's own silhouette from its
	//own outline. Processed one highlighted instance at a time since each needs its own stencil mask; expected
	//to be a small handful of instances at most (this is a selection indicator, not a bulk rendering effect)
	if (!ModelInstance::highlightedInstances.empty())
	{
		pd.shaders->outlineShader->use();
		GLint maskPassUniform = pd.shaders->outlineShader->getUniformLocation("maskPass");

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xFF);

		for (ModelInstance* instance : ModelInstance::highlightedInstances)
		{
			glClear(GL_STENCIL_BUFFER_BIT);

			//Mask pass: mark this instance's own true (non-inflated) silhouette in the stencil buffer, contributing nothing to color
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glCullFace(GL_BACK);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			glUniform1i(maskPassUniform, 1);
			instance->renderSelfOutline();

			//Outline pass: draw the extruded shell everywhere except where the mask above just marked
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glCullFace(GL_FRONT);
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUniform1i(maskPassUniform, 0);
			instance->renderSelfOutline();
		}

		glDisable(GL_BLEND);
		glDisable(GL_STENCIL_TEST);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);
	}

	//Blue tint, slow waves, darker edges, and the scene wobbling like light bending through moving water while the camera is under the water
	if (simulation.waterEnabled && cameraUnderwater)
	{
		//The finished scene is copied out first so underwater.frag can draw it back warped
		int screenWidth = std::max(1, (int)pd.context->getResolution().x);
		int screenHeight = std::max(1, (int)pd.context->getResolution().y);
		if (!pd.underwaterScene || pd.underwaterScene->settings.width != screenWidth || pd.underwaterScene->settings.height != screenHeight)
		{
			RenderTarget::RenderTargetSettings sceneSettings;
			sceneSettings.width = screenWidth;
			sceneSettings.height = screenHeight;
			sceneSettings.channels = 4;
			sceneSettings.useDepth = false;
			sceneSettings.useDepthBuffer = false;
			pd.underwaterScene.reset();
			pd.underwaterScene = std::make_shared<RenderTarget>(sceneSettings, pd.textures);

			//A driver might not resolve the screen into this format, so only the first copy is checked, glGetError can stall every frame
			for (int a = 0; a < 16 && glGetError() != GL_NO_ERROR; a++);
			pd.underwaterScene->copyFromScreen();
			pd.underwaterSceneCopies = pd.underwaterScene->isValid() && glGetError() == GL_NO_ERROR;
			if (!pd.underwaterSceneCopies)
				error("Couldn't copy the screen for the underwater effect, it won't be distorted");
		}
		else if (pd.underwaterSceneCopies)
			pd.underwaterScene->copyFromScreen();

		pd.shaders->underwaterShader->use();
		glUniform1i(pd.shaders->underwaterShader->getUniformLocation("distort"), pd.underwaterSceneCopies);
		if (pd.underwaterSceneCopies)
			pd.underwaterScene->bindColorResult(ScreenCopy);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		//Distorted, it redraws the whole picture itself, otherwise just the tint is blended on top
		if (!pd.underwaterSceneCopies)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		glBindVertexArray(pd.skyVao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	}

	//Covers the scene while picking how our player looks
	if (pd.appearanceEditor->isOpen())
		pd.appearanceEditor->renderPreview(pd.shaders, (int)pd.context->getResolution().x, (int)pd.context->getResolution().y, deltaT);

	//GUI
	bool crossHair = false;
	if (simulation.camera)
		crossHair = pd.context->getMouseLocked() && simulation.camera->getFirstPerson();

	std::vector<std::string> hudLines;
	if (pd.debugMenu->showDebugPhysicsView)
		hudLines.push_back("Debug physics view ON (F2 toggles)");
	if (pd.ghostBrick.isVisible())
	{
		const Brick& ghost = pd.ghostBrick.get();
		std::string material = ghost.material != BrickMaterial_None ? " " + std::string(brickMaterialNames[ghost.material]) : "";
		if (const SpecialBrickType* type = pd.brickTypes.getSpecial(ghost.typeID - 1))
			hudLines.push_back("Ghost brick " + type->uiName + material + ": IJKL move, . , up/down, U rotate, Left Alt super shift, Enter plant, / put away, Ctrl+Z undo");
		else
			hudLines.push_back("Ghost brick " + std::to_string(ghost.width) + "x" + std::to_string(ghost.height) + "x" + std::to_string(ghost.length) + material +
				": IJKL move, . , up/down, U rotate, Left Shift resize, Left Alt super shift, Enter plant, / put away, Ctrl+Z undo");
	}

	pd.escapeMenu->showLeaveServer = client != nullptr;
	pd.gui->superShiftIndicator = pd.ghostBrick.isVisible() ? (pd.ghostBrick.isSuperShift() ? 1 : 0) : -1;
	pd.gui->resizeIndicator = pd.ghostBrick.isVisible() ? (pd.ghostBrick.isResizeMode() ? 1 : 0) : -1;
	pd.gui->voiceIndicator = !client ? -1 : (pd.voice->isTransmitting() ? 1 : (pd.voice->isMuted() ? 0 : -1));
	pd.gui->voiceSpeakers = pd.voice->getSpeaking();
	pd.gui->voiceLevel = pd.voice->getInputLevel();
	pd.gui->voiceClipping = pd.voice->isClipping();
	pd.gui->render(pd.context->getResolution().x, pd.context->getResolution().y,crossHair,hudLines);

	//End frame
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
	//Jets while right mouse is held, only while the mouse is captured for playing rather than clicking around a window
	bool jet = simulation.jetsEnabled && pd.context->getMouseLocked() && !pd.input->supressed && (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK);

	//Go through player controllers, remove any that are bound to now deleted dynamics
	auto ctrlIter = simulation.controllers.begin();
	while (ctrlIter != simulation.controllers.end())
	{
		//Apply movement inputs client side 
		float waterLevel = simulation.waterEnabled ? simulation.waterLevel : PlayerController::noWater;
		if ((*ctrlIter)->control(pd.input, simulation.camera, deltaT, pd.physicsWorld, jet, waterLevel))
		{
			ctrlIter = simulation.controllers.erase(ctrlIter);
			continue;
		}
		else
		{
			if ((*ctrlIter)->jumped)
				pd.audio->playSound("Jump");

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
	//Single player: tick our embedded server before doing any client work this frame.
	//It shares the SimObject::world static with us, so reclaim it for our own PhysicsWorld once it's done.
	if (localServer)
	{
		localServer->run(deltaT, cmdArgs, settings);
		if (pd.physicsWorld)
			SimObject::world = pd.physicsWorld;
	}

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

		//Keeps the sky moving smoothly between the server's once a second WorldStateUpdate packets
		simulation.worldTimeSeconds += (deltaT / 1000.0) * simulation.timeScale;
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

	pd.debugMenu->addExtraLine("Time of day: " + std::to_string(pd.environment.dayFraction) + " (x" + std::to_string(simulation.timeScale) + ")");
	if (simulation.bricks)
		pd.debugMenu->addExtraLine("Bricks: " + std::to_string(simulation.bricks->size()));
	pd.debugMenu->addExtraLine("Environmental audio: " + pd.acousticProbe.getStats());
	pd.debugMenu->addExtraLine("Point lights: " + pd.pointLights->getStats());
	pd.debugMenu->addExtraLine("Particles: " + pd.particles->getStats());
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
		simulation.lights = new ObjHolder<Light>(LightTypeId);
		simulation.emitters = new ObjHolder<Emitter>(EmitterTypeId);
		simulation.bricks = new BrickHolder(pd.physicsWorld, &pd.brickTypes);
		simulation.bricks->setRenderer(pd.brickRenderer);
		simulation.brickDebris = new BrickDebris(pd.physicsWorld, &pd.brickTypes);
		simulation.brickDebris->setLifetime(settings->getFloat("graphics/brickdebrisseconds"));

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

	/*
		Water holds up every dynamic, like on the server, not just the ones this client simulates itself
		Otherwise the local bodies of floating objects fall between server updates and snap back up on each one,
		which shows in the debug physics view and knocks into players swimming around them
	*/
	if (pd.physicsWorld && simulation.dynamics && simulation.waterEnabled)
	{
		for (unsigned int a = 0; a < simulation.dynamics->size(); a++)
			simulation.dynamics->get(a)->applyWaterForces(simulation.waterLevel, deltaT);
	}

	if (pd.physicsWorld)
		pd.physicsWorld->step(deltaT);

	if (simulation.brickDebris)
		simulation.brickDebris->update(deltaT);

	predictLocalCollisions();

	renderEverything(deltaT);

	//The listener is the camera, which rendering just moved for this frame
	glm::vec3 listener = simulation.camera->getPosition();
	bool inWorld = pd.physicsWorld && cmdArgs.gameState == InGame;
	pd.audio->setUnderwater(inWorld && simulation.waterEnabled && listener.y < simulation.waterLevel);

	pd.acousticProbe.updateStats(deltaT);
	if (inWorld && pd.audio->wantsListenerSpace())
	{
		const btRigidBody* player = simulation.controlledDynamics.empty() ? nullptr : simulation.controlledDynamics[0]->body;
		float averageDistance, enclosure;
		if (pd.acousticProbe.measure(deltaT, *pd.physicsWorld, listener, player, averageDistance, enclosure))
			pd.audio->setListenerSpace(averageDistance, enclosure);
	}

	//The Doppler effect goes by the velocity of what the camera follows, so swinging a third person camera around doesn't bend every sound's pitch
	std::optional<glm::vec3> listenerVelocity;
	if (std::shared_ptr<Dynamic> followed = simulation.camera->target.lock())
	{
		btVector3 velocity = followed->getVelocity();
		listenerVelocity = glm::vec3(velocity.x(), velocity.y(), velocity.z());
	}

	//Push to talk is suppressed like every other game key while typing in chat or another window
	bool pushToTalk = client && cmdArgs.gameState == InGame && pd.input->isCommandKeydown(PushToTalk);
	pd.voice->update(pushToTalk, [this](unsigned char flags, uint16_t sequence, const unsigned char* data, unsigned int length)
	{
		if (client)
			client->send(makeVoiceFramePacket(flags, sequence, data, length), VoiceData);
	}, deltaT);

	std::string microphoneProblem;
	if (pd.voice->takeMicrophoneProblem(microphoneProblem))
		pd.gui->addCenterPrint(microphoneProblem, 4000, 1.0f, 0.45f, 0.45f);

	pd.audio->update(listener, simulation.camera->getDirection(), listenerVelocity, deltaT);
}

LoopClient::LoopClient(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	//This will also populate key-bind specific defaults, so we reexport after
	pd.input = std::make_shared<InputMap>(settings);
	settings->exportToFile("Config/settings.txt");

	info("Not dedicated, initalizing client.");

	simulation.idealBufferSize = settings->getInt("network/snapshotbuffer");

	//Created empty the first time, so loading it doesn't log a missing file error
	if (!std::filesystem::exists(ClientProgramData::stateFilePath))
		std::ofstream emptyStateFile(ClientProgramData::stateFilePath);
	pd.state = std::make_shared<SettingManager>(ClientProgramData::stateFilePath);

	//Older builds kept the last server and name in settings.txt
	if (!pd.state->getPreference("network/username") && settings->getPreference("network/username"))
		pd.state->addString("network/username", settings->getString("network/username"));
	if (!pd.state->getPreference("network/lastip") && settings->getPreference("network/lastip"))
		pd.state->addString("network/lastip", settings->getString("network/lastip"));
	if (!pd.state->getPreference("network/lastport") && settings->getPreference("network/port"))
		pd.state->addInt("network/lastport", settings->getInt("network/port"), true, "", 1, 65535);

	//Create our program window
	pd.context = std::make_shared<RenderContext>(settings, pd.state);
	pd.appliedStartResolution = glm::ivec2(settings->getInt("graphics/startresolutionx"), settings->getInt("graphics/startresolutiony"));
	if (!pd.context->isValid())
	{
		//RenderContext already logged exactly what went wrong (window/GL context creation failure). Bail out
		//here instead of continuing on to create ImGui/ShaderManager/etc against a nonexistent GL context,
		//which previously crashed on a null GL function pointer far away from the actual root cause.
		error("Could not create a valid render context, aborting client startup.");
		return;
	}

	//Created before the windows since the brick selector loads its icons through it
	pd.textures = std::make_shared<TextureManager>();

	//Carries on silently if there's no audio device
	pd.audio = std::make_shared<AudioSystem>();
	pd.audio->setVolumes(settings->getFloat("audio/mastervolume"), settings->getFloat("audio/musicvolume"));
	pd.audio->setEnvironmentOptions(settings->getInt("audio/reverbquality"), settings->getInt("audio/occlusionquality"));
	pd.acousticProbe.setQuality(settings->getInt("audio/reverbquality"), settings->getInt("audio/occlusionquality"));
	pd.audio->setVoiceVolume(settings->getFloat("audio/voicevolume"));

	pd.voice = std::make_shared<VoiceChat>(pd.audio);
	pd.voice->setMicrophone(settings->getString("audio/microphone"), settings->getFloat("audio/microphonevolume"));

	//A sound is muffled by anything between it and the camera, other than the player's own body and whatever the sound is on
	pd.audio->setOcclusionTest([this](const glm::vec3& listener, const glm::vec3& source, const btRigidBody* sourceBody)
	{
		if (!pd.physicsWorld)
			return 0.0f;

		const btRigidBody* player = simulation.controlledDynamics.empty() ? nullptr : simulation.controlledDynamics[0]->body;
		return pd.acousticProbe.occlusion(*pd.physicsWorld, listener, source, player, sourceBody);
	});

	pd.brickTypes.load("Assets/brick/types");

	pd.gui = std::make_shared<UserInterface>();
	pd.gui->updateSettings(settings);
	pd.settingsMenu = pd.gui->createWindow<SettingsMenu>(settings, pd.input);
	pd.settingsMenu->setStringChoices("audio/microphone", &VoiceChat::listMicrophones);
	pd.debugMenu = pd.gui->createWindow<DebugMenu>();
	pd.escapeMenu = pd.gui->createWindow<EscapeMenu>();
	pd.serverBrowser = pd.gui->createWindow<ServerBrowser>();
	pd.chatWindow = pd.gui->createWindow<ChatWindow>();
	pd.brickSelector = pd.gui->createWindow<BrickSelector>(&pd.brickTypes, pd.textures);
	pd.brickHotbar = pd.gui->createWindow<BrickHotbar>();
	pd.appearanceEditor = pd.gui->createWindow<AppearanceEditor>(settings, pd.textures, &pd.faceNames);
	pd.wrenchDialog = pd.gui->createWindow<WrenchDialog>();
	//Builds from before the state file kept the hot bar in settings.txt
	std::shared_ptr<SettingManager> hotbarSource = pd.state;
	if (!pd.state->getPreference("hotbar/slot1/filled") && settings->getPreference("hotbar/slot1/filled"))
		hotbarSource = settings;
	pd.brickSelector->load(hotbarSource);
	pd.brickHotbar->load(hotbarSource, [this](const std::string& brickName) { return pd.brickSelector->findIcon(brickName); });
	pd.brickSelector->save(pd.state);
	pd.brickHotbar->save(pd.state);
	pd.state->exportToFile(ClientProgramData::stateFilePath);

	//Anything moved to the state file comes out of settings.txt, otherwise it would linger in the settings menu doing nothing
	bool removedFromSettings = settings->remove("hotbar");
	for (const char* moved : { "network/username", "network/lastip", "network/port" })
		removedFromSettings = settings->remove(moved) || removedFromSettings;
	//Replaced by audio/reverbquality and audio/occlusionquality
	for (const char* replaced : { "audio/environmentalreverb", "audio/occlusion", "audio/raycastquality" })
		removedFromSettings = settings->remove(replaced) || removedFromSettings;
	if (removedFromSettings)
		settings->exportToFile("Config/settings.txt");

	std::string lastIp = pd.state->getPreference("network/lastip") ? pd.state->getString("network/lastip") : "localhost";
	int lastPort = pd.state->getPreference("network/lastport") ? pd.state->getInt("network/lastport") : DEFAULT_PORT;
	std::string lastName = pd.state->getPreference("network/username") ? pd.state->getString("network/username") : "Guest";
	pd.serverBrowser->passDefaultSettings(lastIp, lastPort, lastName);
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

	//Faces players pick in the appearance editor, each on its own decal array layer, servers send them by file name
	std::vector<std::filesystem::path> facePaths;
	if (std::filesystem::is_directory("Assets/faces"))
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("Assets/faces"))
		{
			std::string extension = lowercase(entry.path().extension().string());
			if (entry.is_regular_file() && (extension == ".png" || extension == ".jpg"))
				facePaths.push_back(entry.path());
		}
	}
	std::sort(facePaths.begin(), facePaths.end());

	//Decal IDs get 8 bits of a mesh's instance flags
	if (facePaths.size() > 256)
		facePaths.resize(256);

	pd.textures->allocateForDecals(256, std::max<unsigned int>(1, (unsigned int)facePaths.size()));
	for (const std::filesystem::path& facePath : facePaths)
	{
		if (pd.textures->addDecal(facePath.generic_string(), (int)pd.faceNames.size()))
			pd.faceNames.push_back(facePath.filename().string());
	}
	pd.textures->finalizeDecals();

	pd.grassMaterial = new Material("Assets/ground/grass.txt", pd.textures);

	if (!pd.grassMaterial->isValid())
	{
		pd.gui->popupErrorMessage = "Error loading grass material, see error log.";
	}

	pd.grassVao = createQuadVAO();

	pd.pointLights = new PointLights();
	pd.particles = new ParticleSystem(pd.textures);
	pd.particles->setMaxParticles(settings->getInt("graphics/maxparticles"));
	createShadowTarget(settings);
	pd.lightSpaceMatriciesUniformModel = pd.shaders->modelShader->getUniformLocation("lightSpaceMatricies");
	pd.lightSpaceMatriciesUniformBrick = pd.shaders->brickShader->getUniformLocation("lightSpaceMatricies");
	pd.shadowCascadeMatrixUniformModel = pd.shaders->modelShadowCascadeShader->getUniformLocation("lightSpaceMatrix");
	pd.shadowCascadeMatrixUniformBrick = pd.shaders->brickShadowCascadeShader->getUniformLocation("lightSpaceMatrix");
	pd.shadowTintMatrixUniform = pd.shaders->brickShadowTintShader->getUniformLocation("lightSpaceMatrix");
	pd.shadowCascadeMinOpacityUniform = pd.shaders->brickShadowCascadeShader->getUniformLocation("minOpacity");
	pd.shadowTintMinOpacityUniform = pd.shaders->brickShadowTintShader->getUniformLocation("minOpacity");
	pd.shadowCascadeSkipContainingUniform = pd.shaders->brickShadowCascadeShader->getUniformLocation("skipContaining");
	pd.shadowCascadeSkipPointUniform = pd.shaders->brickShadowCascadeShader->getUniformLocation("skipPoint");
	pd.shadowTintSkipContainingUniform = pd.shaders->brickShadowTintShader->getUniformLocation("skipContaining");
	pd.shadowTintSkipPointUniform = pd.shaders->brickShadowTintShader->getUniformLocation("skipPoint");

	pd.brickRenderer = new InstancedBrickRenderer(pd.shaders, pd.textures, &pd.brickTypes);

	glGenVertexArrays(1, &pd.skyVao);

	//Integer grid coordinates go through the same math for every cell that shares a vertex, so no cracks between cells
	auto gridPoint = [](int x, int z) { return glm::vec2(x, z) / (float)waterGridCells * 2.0f - 1.0f; };
	std::vector<glm::vec2> waterGrid;
	waterGrid.reserve(waterGridCells * waterGridCells * 6);
	for (int x = 0; x < waterGridCells; x++)
	{
		for (int z = 0; z < waterGridCells; z++)
		{
			waterGrid.push_back(gridPoint(x, z));
			waterGrid.push_back(gridPoint(x, z + 1));
			waterGrid.push_back(gridPoint(x + 1, z));
			waterGrid.push_back(gridPoint(x + 1, z));
			waterGrid.push_back(gridPoint(x, z + 1));
			waterGrid.push_back(gridPoint(x + 1, z + 1));
		}
	}
	pd.waterVertexCount = (GLsizei)waterGrid.size();

	glGenVertexArrays(1, &pd.waterVao);
	glBindVertexArray(pd.waterVao);
	glGenBuffers(1, &pd.waterVbo);
	glBindBuffer(GL_ARRAY_BUFFER, pd.waterVbo);
	glBufferData(GL_ARRAY_BUFFER, waterGrid.size() * sizeof(glm::vec2), waterGrid.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindVertexArray(0);

	createWaterTargets(settings);

	info("Start up complete");

	printAllGraphicsErrors("End of initalization");

	valid = true;

	//Automated testing convenience: skip the server browser and get straight into a game, same as clicking "Start Server"
	if (cmdArgs.autoSinglePlayer)
		hostSinglePlayer(cmdArgs, settings);
}

LoopClient::~LoopClient()
{
	//testChunk.deleteAllBricks();

	pd.shadows.reset();
	pd.shadowTint.reset();
	pd.waterReflection.reset();
	pd.waterRefraction.reset();
	pd.underwaterScene.reset();

	delete pd.grassMaterial;
	glDeleteVertexArrays(1, &pd.grassVao);
	glDeleteVertexArrays(1, &pd.skyVao);
	glDeleteVertexArrays(1, &pd.waterVao);
	glDeleteBuffers(1, &pd.waterVbo);

	delete pd.brickRenderer;
	pd.brickRenderer = nullptr;

	delete pd.pointLights;
	pd.pointLights = nullptr;

	delete pd.particles;
	pd.particles = nullptr;

	//Its model and picking target need the OpenGL context, which goes away with pd.context
	if (pd.appearanceEditor)
		pd.appearanceEditor->releaseGraphics();

	//Not needed this is a destructor lol
	//Also this should only be called when the programs shutting down anyway
	pd.context.reset();
	pd.gui.reset(); //Will handle indivdual windows
	pd.shaders.reset();
	pd.textures.reset();
	pd.voice.reset();
	pd.audio.reset();

	//This one is actually useful because the server will learn we disconnected faster if we do it properly
	if(client)
		delete client;

	if (localServer)
		delete localServer;
}
