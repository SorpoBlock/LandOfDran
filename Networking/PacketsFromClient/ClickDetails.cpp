#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../LuaFunctions/ClientLua.h"
#include "../../LuaFunctions/SoundLua.h"
#include "../../LuaFunctions/VehicleLua.h"

#include <cmath>

//How far from the camera a vehicle can be right clicked to get in, like the old game
static constexpr float vehicleReach = 30.0f;

//Honking while driving waits this long between honks, like the old game
static constexpr unsigned int honkCooldownMS = 1000;

/*
	Do not attempt to assign a handle to JoinedClient to other objects directly
	Grab a smart pointer from the server for this
	server->getClientByNetId(source->getNetId());
	Also do not attempt to delete the JoinedClient, just call kick
*/
void clickDetails(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (packet->dataLength < 2 + sizeof(float) * 6)
		return;

	int byteIteartor = 1;

	glm::vec3 pos, dir;

	memcpy(&pos, packet->data + byteIteartor, sizeof(glm::vec3));
	byteIteartor += sizeof(glm::vec3);

	memcpy(&dir, packet->data + byteIteartor, sizeof(glm::vec3));
	byteIteartor += sizeof(glm::vec3);

	unsigned char mask = packet->data[byteIteartor];
	byteIteartor++;

	unsigned char clickFlags = byteIteartor < packet->dataLength ? packet->data[byteIteartor] : 0;
	bool release = clickFlags & ClickFlag_Release;

	std::shared_ptr<ClientData> client = pd->getClient(source->me);
	bool usable = client && !release && std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) && std::isfinite(dir.x) && std::isfinite(dir.y) && std::isfinite(dir.z) && glm::length(dir) > 0.0001f;

	if (usable && (clickFlags & ClickFlag_RightPress))
	{
		//Right click gets out of a vehicle, or into the one under the crosshair, like the old game
		if (!client->vehicle.expired())
		{
			exitVehicle(*client, true);
			return;
		}

		btRigidBody* ignore = client->controlledObjects.empty() ? nullptr : client->controlledObjects[0]->body;
		btRigidBody* hit = pd->physicsWorld->doRaycast(g2b3(pos), g2b3(pos + glm::normalize(dir) * vehicleReach), ignore);
		if (std::shared_ptr<Vehicle> vehicle = vehicleFromBody(hit))
		{
			if (enterVehicle(*client, vehicle, true))
				return;
		}
	}
	else if (usable && (clickFlags & ClickFlag_LeftPress))
	{
		//Left click honks while driving, if Lua registered the old game's Honk sound
		std::shared_ptr<Vehicle> vehicle = client->vehicle.lock();
		if (vehicle && vehicle->body && SDL_GetTicks() - client->lastHonkMS > honkCooldownMS)
		{
			client->lastHonkMS = SDL_GetTicks();
			playSoundAt("Honk", b2g3(vehicle->body->getWorldTransform().getOrigin()), 1.0f, 1.0f);
		}
	}

	pushClientLua(pd->luaState, source->me);
	lua_pushnumber(pd->luaState, pos.x);
	lua_pushnumber(pd->luaState, pos.y);
	lua_pushnumber(pd->luaState, pos.z);
	lua_pushnumber(pd->luaState, dir.x);
	lua_pushnumber(pd->luaState, dir.y);
	lua_pushnumber(pd->luaState, dir.z);
	lua_pushnumber(pd->luaState, mask);
	pd->eventManager->callEvent(pd->luaState, release ? "ClientClickRelease" : "ClientClick", 8);

	//Either return values will be correct, or they will be zero, do nothing special if lua functions messed up the event
	if (lua_gettop(pd->luaState) != 0)
	{
		//Nothing specific to do at the moment for a click, it's all in the scripts
		lua_settop(pd->luaState, 0);
	}
}
