#include "Server.h"
#include "../GameLoop/ServerProgramData.h"
#include "../LuaFunctions/ClientLua.h"

void handleDisconnect(JoinedClient* client, Server* server, const void* pdv, lua_State* L, EventManager* eventManager)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (client->name.length() > 0)
	{
		//Call event to let Lua scripts do whatever it needs for the client before it's removed
		pushClientLua(L, client->me);
		eventManager->callEvent(L, "ClientLeave", 1);
		lua_settop(L, 0); //Don't need to do anything with returned client object

		if (client->userData)
			pd->removeClient(client->me);

		std::string message = client->name + " disconnected.";
		server->broadcastChat(message);
		info(message);
	}
}
