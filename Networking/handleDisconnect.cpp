#include "Server.h"
#include "../GameLoop/ServerProgramData.h"

void handleDisconnect(JoinedClient* client, Server* server, const void* pdv, lua_State* L, EventManager* eventManager)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (client->userData)
		pd->removeClient(client->me);

	if (client->name.length() > 0)
	{
		lua_pushnumber(L, client->getNetId());
		lua_pushstring(L, client->name.c_str());
		eventManager->callEvent(L, "ClientLeave", 2);
		lua_settop(L, 0); //Don't need to do anything with returned ID or name

		std::string message = client->name + " disconnected.";
		server->broadcastChat(message);
		info(message);
	}
}
