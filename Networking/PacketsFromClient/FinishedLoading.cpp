#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../LuaFunctions/ClientLua.h"

/*
	Do not attempt to assign a handle to JoinedClient to other objects directly
	Grab a smart pointer from the server for this
	server->getClientByNetId(source->getNetId());
	Also do not attempt to delete the JoinedClient, just call kick
*/
void clientFinishedLoading(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	pushClientLua(pd->luaState, source->me);
	pd->eventManager->callEvent(pd->luaState, "ClientJoin", 1);
	lua_settop(pd->luaState, 0); //We don't need to do anything with the client the lua function returns

	info(source->name + " finished loading phase 1");

	//They finished loading types, now send pre-existing SimObjects
	pd->dynamics->sendAll(source);
	pd->statics->sendAll(source);
}
