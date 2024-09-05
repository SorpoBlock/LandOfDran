#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../LuaFunctions/ClientLua.h"

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

	pushClientLua(pd->luaState, source->me);
	lua_pushnumber(pd->luaState, pos.x);
	lua_pushnumber(pd->luaState, pos.y);
	lua_pushnumber(pd->luaState, pos.z);
	lua_pushnumber(pd->luaState, dir.x);
	lua_pushnumber(pd->luaState, dir.y);
	lua_pushnumber(pd->luaState, dir.z);
	lua_pushnumber(pd->luaState, mask);
	pd->eventManager->callEvent(pd->luaState, "ClientClick", 8);

	//Either return values will be correct, or they will be zero, do nothing special if lua functions messed up the event
	if (lua_gettop(pd->luaState) != 0)
	{
		//Nothing specific to do at the moment for a click, it's all in the scripts
		lua_settop(pd->luaState, 0);
	}
}
