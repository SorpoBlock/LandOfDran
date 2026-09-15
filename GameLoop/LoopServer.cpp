#include "LoopServer.h"

#include "../LuaFunctions/Dynamic.h"
#include "../LuaFunctions/Static.h"
#include "../LuaFunctions/LightLua.h"
#include "../LuaFunctions/SoundLua.h"
#include "../LuaFunctions/EmitterLua.h"
#include "../LuaFunctions/BrickLua.h"
#include "../LuaFunctions/SkyLua.h"

#include <random>

void LoopServer::run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	//When embedded alongside a LoopClient in the same process (single player), both loops
	//share this one static pointer. Reassert ours here since the client may have pointed it
	//at its own PhysicsWorld since our last tick.
	SimObject::world = pd.physicsWorld;

	if (deltaT > slowestTickMS)
		slowestTickMS = deltaT;

	totalTicks++;
	totalTicksMS += deltaT;

	if (SDL_GetTicks() - slowestTickCounter > 1000)
	{
		slowestTickCounter = SDL_GetTicks();

		float averageTickMS = totalTicksMS / totalTicks;

		ENetPacket* ret = enet_packet_create(NULL, 1 + sizeof(float) * 2, getFlagsFromChannel(OtherReliable));
		ret->data[0] = (unsigned char)ServerPerformanceDetails;
		memcpy(ret->data + 1, &lastSlowestTickMS, sizeof(float));
		memcpy(ret->data + 1 + sizeof(float), &averageTickMS, sizeof(float));

		server->broadcast(ret, OtherReliable);

		totalTicks = 0;
		totalTicksMS = 0;
		lastSlowestTickMS = slowestTickMS;
		slowestTickMS = 0;
	}

	pd.worldTimeSeconds += (deltaT / 1000.0) * pd.timeScale;
	if (pd.worldStateChanged || SDL_GetTicks() - lastWorldStateBroadcast > 1000)
		broadcastWorldState();

	server->run(&pd,pd.luaState,pd.eventManager); //   <---- networking
	endQuietTalkers();
	pd.dynamics->sendRecent();
	pd.statics->sendRecent();
	pd.lights->sendRecent();
	pd.emitters->sendRecent();
	pd.bricks->sendRecent();
	applyWaterForces(deltaT);
	pd.physicsWorld->step(deltaT);

	playWaterSounds();
	updateEmitters();

	for (unsigned int a = 0; a < Logger::getStorage()->size(); a++)
		server->updateAdminConsoles(Logger::getStorage()->at(a));
	Logger::getStorage()->clear();

	for (unsigned int a = 0; a < pd.clients.size(); a++)
	{
		for (unsigned int b = 0; b < pd.clients[a]->controllers.size(); b++)
		{
			pd.clients[a]->controllers[b].controlWithLastInput(pd.physicsWorld, deltaT, pd.waterEnabled ? pd.waterLevel : PlayerController::noWater);
		}
	}

	updatePlayerAbilities();

	//Drive any dynamics currently snapped to a client's cursor (see dynamic:snapToCursor)
	for (unsigned int a = 0; a < pd.dynamics->size(); a++)
	{
		std::shared_ptr<Dynamic> dynamic = pd.dynamics->get(a);
		if (!dynamic->isSnappedToCursor())
			continue;

		std::shared_ptr<JoinedClient> owner = dynamic->snappedToClient.lock();
		std::shared_ptr<ClientData> ownerData = owner ? pd.getClient(owner) : nullptr;

		//Owning client disconnected (or otherwise lost its controller) since this was snapped - drop it back into normal physics
		if (!ownerData || ownerData->controllers.size() == 0)
		{
			dynamic->unsnapFromCursor();
			continue;
		}

		dynamic->updateCursorSnapPosition(ownerData->controllers[0].lastCameraPosition, ownerData->controllers[0].lastCameraDirection);
	}

	scheduler->run(pd.luaState);
}

void LoopServer::endQuietTalkers()
{
	unsigned int now = SDL_GetTicks();

	for (unsigned int a = 0; a < pd.clients.size(); a++)
	{
		std::shared_ptr<ClientData> talker = pd.clients[a];
		if (!talker->talking || now - talker->lastVoiceMS < ServerProgramData::voiceTimeoutMS)
			continue;

		talker->talking = false;

		pushClientLua(pd.luaState, talker->client);
		pd.eventManager->callEvent(pd.luaState, "ClientStopTalking", 1);
		lua_settop(pd.luaState, 0);
	}
}

void LoopServer::applyWaterForces(float deltaT)
{
	if (!pd.waterEnabled)
		return;

	/*
		Includes the dynamics clients simulate themselves: their physics packets only come every 100 ms or so, and without
		water in between the server would drop them and snap them back up, which everyone else would see
	*/
	for (unsigned int a = 0; a < pd.dynamics->size(); a++)
	{
		std::shared_ptr<Dynamic> dynamic = pd.dynamics->get(a);
		if (dynamic->isSnappedToCursor())
			continue;

		dynamic->applyWaterForces(pd.waterLevel, deltaT);
	}
}

void LoopServer::playWaterSounds()
{
	//Vertical speeds, world units per second, below which going in or out of the water is silent
	static constexpr float splashSpeed = 6.0f;
	static constexpr float exitSpeed = 6.0f;
	static constexpr unsigned int soundCooldownMS = 700;

	static std::mt19937 random(std::random_device{}());
	std::uniform_real_distribution<float> pitchVariation(0.9f, 1.1f);

	for (unsigned int a = 0; a < pd.dynamics->size(); a++)
	{
		std::shared_ptr<Dynamic> dynamic = pd.dynamics->get(a);

		if (!pd.waterEnabled)
		{
			dynamic->inWater = false;
			continue;
		}

		btVector3 aabbMin, aabbMax;
		dynamic->body->getAabb(aabbMin, aabbMax);

		//A little gap between going in and coming out, so something floating at the surface doesn't keep doing both
		bool wasInWater = dynamic->inWater;
		if (!wasInWater && aabbMin.y() < pd.waterLevel)
			dynamic->inWater = true;
		else if (wasInWater && aabbMin.y() > pd.waterLevel + 0.5f)
			dynamic->inWater = false;

		if (dynamic->inWater == wasInWater || SDL_GetTicks() - dynamic->lastWaterSoundMS < soundCooldownMS)
			continue;

		float verticalSpeed = dynamic->getVelocity().y();
		btVector3 center = (aabbMin + aabbMax) * 0.5f;
		glm::vec3 surface(center.x(), pd.waterLevel, center.z());

		//Bigger things sound deeper, and no two splashes sound quite the same
		float size = (aabbMax - aabbMin).length();
		float pitch = std::clamp(1.3f - size * 0.06f, 0.6f, 1.3f) * pitchVariation(random);

		if (dynamic->inWater && -verticalSpeed > splashSpeed)
		{
			playSoundAt("Splash", surface, pitch, std::clamp(0.3f + (-verticalSpeed - splashSpeed) / 40.0f, 0.3f, 1.0f));
			//The old game's splash effect, if Lua defined it
			spawnEmitterAt("playerBubbleEmitter", surface);
			dynamic->lastWaterSoundMS = SDL_GetTicks();
		}
		else if (!dynamic->inWater && verticalSpeed > exitSpeed)
		{
			playSoundAt("ExitWater", surface, pitch, std::clamp(0.2f + (verticalSpeed - exitSpeed) / 60.0f, 0.2f, 0.7f));
			dynamic->lastWaterSoundMS = SDL_GetTicks();
		}
	}
}

void LoopServer::updateEmitters()
{
	unsigned int now = SDL_GetTicks();

	//Backwards, since destroying one moves the ones after it down
	for (int a = (int)pd.emitters->size() - 1; a >= 0; a--)
	{
		std::shared_ptr<Emitter> emitter = pd.emitters->get(a);

		uint16_t typeID = emitter->getTypeID();
		//Ones on bricks last as long as the brick, their particles still live out their own lifetimes
		bool expired = emitter->getAttachKind() != EmitterAttachBrick && typeID < pd.emitterTypes.size() && pd.emitterTypes[typeID].lifetimeMS > 0 && now - emitter->getCreationTime() > pd.emitterTypes[typeID].lifetimeMS;
		bool dynamicGone = emitter->getAttachKind() == EmitterAttachDynamic && emitter->dynamic.expired();
		bool brickGone = emitter->brickID != NO_ID && !pd.bricks->find(emitter->brickID);

		if (expired || dynamicGone || brickGone)
			pd.emitters->destroy(emitter);
	}
}

void LoopServer::updatePlayerAbilities()
{
	//How far where a client looks has to turn before their flashlight turns with it, so holding still doesn't keep sending updates
	static const float flashlightTurnCosine = std::cos(glm::radians(1.0f));
	static const char* feet[2] = { "Left_Foot", "Right_Foot" };

	for (std::shared_ptr<ClientData>& client : pd.clients)
	{
		for (PlayerController& controller : client->controllers)
		{
			std::shared_ptr<Dynamic> target = controller.target.lock();
			bool jetting = target && controller.lastJet && controller.jetsAllowed;
			bool flaming = !controller.jetEmitters[0].expired() || !controller.jetEmitters[1].expired();

			if (jetting && !flaming)
			{
				//The old game's flames under each foot, if Lua defined them
				for (int a = 0; a < 2; a++)
				{
					int mesh = target->getType()->getModel()->getMeshIdx(feet[a]);
					//A model without feet gets one from its middle
					if (mesh == -1 && a == 1)
						break;

					std::shared_ptr<Emitter> flame = spawnEmitterAt("playerJetEmitter", b2g3(target->getPosition()));
					if (!flame)
						break;

					flame->attachToDynamic(target, mesh);
					controller.jetEmitters[a] = flame;
				}
			}
			else if (!jetting && flaming)
			{
				for (std::weak_ptr<Emitter>& jet : controller.jetEmitters)
				{
					if (std::shared_ptr<Emitter> flame = jet.lock())
						pd.emitters->destroy(flame);
					jet.reset();
				}
			}
		}

		std::shared_ptr<Light> light = client->flashlight.lock();
		if (!light)
			continue;

		std::shared_ptr<Dynamic> holder = client->controllers.empty() ? nullptr : client->controllers[0].target.lock();
		if (!holder)
		{
			client->setFlashlight(&pd, false, light->getColor());
			continue;
		}

		glm::vec3 look = client->controllers[0].lastCameraDirection;
		if (glm::length(look) > 0.0001f && glm::dot(glm::normalize(look), light->getDirection()) < flashlightTurnCosine)
			light->setDirection(look);
	}
}

void LoopServer::broadcastWorldState()
{
	lastWorldStateBroadcast = SDL_GetTicks();
	pd.worldStateChanged = false;

	ENetPacket* packet = enet_packet_create(NULL, 1 + sizeof(double) + sizeof(float) * 2 + 1 + sizeof(float) * DayCycle::networkFloats, getFlagsFromChannel(OtherReliable));
	enet_uint8* data = packet->data;

	data[0] = (unsigned char)WorldStateUpdate;
	data++;

	memcpy(data, &pd.worldTimeSeconds, sizeof(double));
	data += sizeof(double);

	memcpy(data, &pd.timeScale, sizeof(float));
	data += sizeof(float);

	memcpy(data, &pd.waterLevel, sizeof(float));
	data += sizeof(float);

	data[0] = pd.waterEnabled ? 1 : 0;
	data++;

	auto writeColor = [&data](const glm::vec3& color)
	{
		memcpy(data, &color[0], sizeof(float) * 3);
		data += sizeof(float) * 3;
	};
	for (const SkyKeyframe& phase : pd.dayCycle.phases)
	{
		writeColor(phase.skyColor);
		writeColor(phase.fogColor);
		writeColor(phase.lightColor);
		writeColor(phase.ambientColor);
	}

	memcpy(data, &pd.dayCycle.fogStart, sizeof(float));
	data += sizeof(float);

	memcpy(data, &pd.dayCycle.fogEnd, sizeof(float));

	server->broadcast(packet, OtherReliable);
}

LoopServer::LoopServer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server = new Server(DEFAULT_PORT);
	LUA_server = server;
	if (!server->isValid())
		return;

	LUA_args = &cmdArgs;
	LUA_pd = &pd;

	pd.evalPassword = settings->getString("hosting/evalpassword");
	pd.useEvalPassword = settings->getBool("hosting/useevalpassword");
	if (pd.useEvalPassword && (pd.evalPassword == " " || pd.evalPassword == "changeme" || pd.evalPassword.length() < 1))
	{
		error("Eval password protection is enabled, but the password is still the default 'changeme' (or empty). Set a real password in Settings, under Hosting, then restart the server. Eval console logins are refused until then.");
		pd.useEvalPassword = false;
	}

	//Not a dedicated server means we're embedded in the graphical client (single player/"Start Server"), so the
	//only client that can reach us over loopback is our own host - let them straight into the eval console
	pd.autoAdminForLoopback = !cmdArgs.dedicated;
	//TODO: Hash password

	//Start up Lua and give it access to all the default libraries, file io, debugging, math, etc.
	pd.luaState = luaL_newstate();
	luaL_openlibs(pd.luaState);
	registerOtherFunctions(pd.luaState);
	scheduler = new LuaScheduler(pd.luaState);
	pd.eventManager = new EventManager(pd.luaState);
	registerClientFunctions(pd.luaState);
	registerSoundFunctions(pd.luaState);
	registerSkyFunctions(pd.luaState);

	///Server just has one physics world that's started when the program starts and stays until shutdown, unlike client
	pd.physicsWorld = std::make_shared<PhysicsWorld>();
	SimObject::world = pd.physicsWorld;

	pd.dynamics = new ObjHolder<Dynamic>(SimObjectType::DynamicTypeId, server);
	pd.dynamics->makeLuaMetatable(pd.luaState, "metatable_dynamic", getDynamicFunctions(pd.luaState));
	pd.statics = new ObjHolder<StaticObject>(SimObjectType::StaticTypeId, server);
	pd.statics->makeLuaMetatable(pd.luaState, "metatable_static", getStaticFunctions(pd.luaState));
	pd.lights = new ObjHolder<Light>(SimObjectType::LightTypeId, server);
	pd.lights->makeLuaMetatable(pd.luaState, "metatable_light", getLightFunctions(pd.luaState));
	pd.emitters = new ObjHolder<Emitter>(SimObjectType::EmitterTypeId, server);
	pd.emitters->makeLuaMetatable(pd.luaState, "metatable_emitter", getEmitterFunctions(pd.luaState));
	pd.bricks = new BrickHolder(pd.physicsWorld, &pd.brickTypes, server);
	//Music, lights, and emitters put on bricks come and go with them
	pd.bricks->spawnAttachments = updateBrickAttachments;
	pd.bricks->removeAttachments = removeBrickAttachments;
	pd.brickTypes.load("Assets/brick/types");
	pd.bricks->makeLuaMetatable(pd.luaState, "metatable_brick", getBrickFunctions(pd.luaState));

	info("Loading serverstart.lua");

	if (luaL_dofile(pd.luaState, "serverstart.lua"))
	{
		error("Error loading serverstart.lua: " + std::string(lua_tostring(pd.luaState, -1)));

		//Only block on console input for a real standalone dedicated server.
		//When embedded in the graphical client (single player), there's no console to read from.
		if (cmdArgs.dedicated)
		{
			info("Input any text to exit.");
			std::string holdForAWhile;
			std::cin >> holdForAWhile;
		}

		valid = false;
		return;
	}

	info("Server running");

	if(pd.allNetTypes.size() == 0)
		error("No NetTypes registered, clients will freeze on joining, no objects can be created.");

	valid = true;
}

LoopServer::~LoopServer()
{
	delete pd.eventManager;
	delete scheduler;

	LUA_args = nullptr;
	LUA_pd = nullptr;

	//Removes brick bodies, so it has to go before the physics world
	delete pd.bricks;
	pd.bricks = nullptr;

	pd.physicsWorld.reset();
	SimObject::world = nullptr;

	delete pd.dynamics;
	delete pd.statics;
	delete pd.lights;
	delete pd.emitters;

	pd.dynamicTypes.clear();
	pd.allNetTypes.clear();

	delete server;
	LUA_server = nullptr;
}
