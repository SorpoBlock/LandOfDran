#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../LuaFunctions/ClientLua.h"
#include "../../LuaFunctions/BrickLua.h"
#include "../../LuaFunctions/SoundLua.h"
#include "../../LuaFunctions/EmitterLua.h"

#include <cmath>

//How far from the camera a brick can be wrenched
static constexpr float wrenchReach = 100.0f;

/*
	1 byte		-	packet type
	12 bytes	-	camera position
	12 bytes	-	camera direction

	Wrenches the brick the camera points at, which opens the wrench dialog for it unless a ClientWrenchBrick listener says not to
*/
void wrenchRequest(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (packet->dataLength < 1 + sizeof(float) * 6)
		return;

	float values[6];
	memcpy(values, packet->data + 1, sizeof(values));
	for (float value : values)
		if (!std::isfinite(value))
			return;

	glm::vec3 start(values[0], values[1], values[2]);
	glm::vec3 direction(values[3], values[4], values[5]);
	if (glm::length(direction) < 0.0001f)
		return;
	direction = glm::normalize(direction);

	std::shared_ptr<ClientData> client = pd->getClient(source->me);
	if (!client)
		return;

	//Not their own player, a third person camera looks past it
	btRigidBody* ignore = client->controlledObjects.empty() ? nullptr : client->controlledObjects[0]->body;
	btRigidBody* hit = pd->physicsWorld->doRaycast(g2b3(start), g2b3(start + direction * wrenchReach), ignore);
	if (!hit || hit->getUserIndex() != brickBody)
		return;

	Brick* brick = (Brick*)hit->getUserPointer();
	netIDType brickID = brick->netId;

	lua_State* L = pd->luaState;
	pushClientLua(L, source->me);
	pd->bricks->pushLua(L, brick);
	pd->eventManager->callEvent(L, "ClientWrenchBrick", 2);

	//A listener can return nil instead of the brick to keep the dialog closed, or another brick to open that one's instead
	//One that broke leaves nothing on the stack, which doesn't stop wrenching
	if (lua_gettop(L) == 2)
	{
		if (lua_isnil(L, 2))
		{
			lua_settop(L, 0);
			return;
		}

		brick = pd->bricks->popLua(L);
	}
	else
		brick = pd->bricks->find(brickID);

	lua_settop(L, 0);

	if (brick)
		openWrenchDialog(*client, brick);
}

/*
	1 byte		-	packet type
	4 bytes		-	brick net ID
	1 byte		-	1 if it collides
	1 byte		-	name length
	0-255 bytes	-	name
	The rest	-	BrickAttachments::write
*/
void wrenchSubmit(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (packet->dataLength < 2 + sizeof(netIDType) + 1)
		return;

	std::shared_ptr<ClientData> client = pd->getClient(source->me);
	if (!client)
		return;

	netIDType brickID;
	memcpy(&brickID, packet->data + 1, sizeof(netIDType));

	//Only the brick they were last sent a dialog for, and only once
	if (brickID != client->wrenchedBrickID)
		return;
	client->wrenchedBrickID = NO_ID;

	Brick* brick = pd->bricks->find(brickID);
	if (!brick)
		return;

	bool collides = packet->data[1 + sizeof(netIDType)] & 1;

	size_t at = 2 + sizeof(netIDType);
	size_t nameLength = packet->data[at++];
	if (at + nameLength > packet->dataLength)
		return;

	std::string name((char*)packet->data + at, nameLength);
	at += nameLength;

	BrickAttachments settings;
	if (!settings.read(packet->data, packet->dataLength, at))
		return;

	//Players pick from the server's music and emitter types, but whatever Lua put on the brick can stay
	const BrickAttachments* current = brick->attachments.get();
	if (!settings.musicName.empty() && !isMusicSoundType(settings.musicName) && !(current && current->musicName == settings.musicName))
		settings.musicName = "";
	if (!settings.emitterName.empty() && !emitterTypeExists(settings.emitterName) && !(current && current->emitterName == settings.emitterName))
		settings.emitterName = "";

	if (brick->collides != collides)
		pd->bricks->setColliding(brick, collides);
	brick->name = name;
	setBrickAttachments(brick, settings);
}
