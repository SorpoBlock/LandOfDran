#include "ClientLua.h"

Server * LUA_server = nullptr;

void pushClientLua(lua_State* L, std::shared_ptr<JoinedClient> client)
{
	std::string metatableName = "metatable_client";

	//Set class metatable
	lua_newtable(L);
	lua_getglobal(L, metatableName.c_str());
	lua_setmetatable(L, -2);

	//Push class-unique server ID
	lua_pushinteger(L, client->getNetId());
	lua_setfield(L, -2, "id");

	//Allow unambigious type checking
	lua_pushinteger(L, ClientTypeId);
	lua_setfield(L, -2, "type");

	//Lua will automatically deallocate the space for the weak_ptr itself when the table is garbage collected
	std::weak_ptr<JoinedClient>* userdata = (std::weak_ptr<JoinedClient>*)lua_newuserdata(L, sizeof(std::weak_ptr<JoinedClient>));
	new(userdata) std::weak_ptr<JoinedClient>(client->me);
	lua_setfield(L, -2, "ptr");
}

std::shared_ptr<JoinedClient> popClientLua(lua_State* L)
{
	scope("popLua");

	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		error("No table for supposed JoinedClient");
		return nullptr;
	}

	lua_getfield(L, -1, "type");
	if (!lua_isinteger(L, -1))
	{
		lua_pop(L, 2);
		error("No type field for supposed JoinedClient");
		return nullptr;
	}

	SimObjectType poppedType = (SimObjectType)lua_tointeger(L, -1);
	lua_pop(L, 1);

	if (poppedType != ClientTypeId)
	{
		lua_pop(L, 1);
		error("JoinedClient type mismatch: " + std::to_string(poppedType)+", invalid Lua cast");
		return nullptr;
	}

	lua_getfield(L, -1, "ptr");
	if (!lua_isuserdata(L, -1))
	{
		lua_pop(L, 2);
		error("No ptr field for supposed JoinedClient");
		return nullptr;
	}

	std::weak_ptr<JoinedClient>* obj = (std::weak_ptr<JoinedClient>*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (!obj)
	{
		lua_pop(L, 1);
		error("Lua userdata for JoinedClient weak_ptr not set");
		return nullptr;
	}

	if (obj->expired())
	{
		lua_getfield(L, -1, "id");
		if (lua_isinteger(L, -1))
			error("JoinedClient with ID " + std::to_string(lua_tointeger(L, -1)) + " was deleted");
		else
			error("JoinedClient was deleted, could not get ID");

		lua_pop(L, 2);
		return nullptr;
	}

	return obj->lock();
}

static int LUA_clientKick(lua_State* L)
{
	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument client:kick()");
		return 0;
	}

	std::shared_ptr<JoinedClient> client = popClientLua(L);
	client->kick(KickReason::LuaKick);

	return 0;
}

static int LUA_clientMessage(lua_State* L)
{
	if (lua_gettop(L) != 2)
	{
		error("Expected 2 arguments client:message(message)");
		return 0;
	}

	const char* msg = lua_tostring(L, -1);
	lua_pop(L, 1);

	if (!msg)
	{
		error("Expected string for message");
		return 0;
	}

	std::string message = std::string(msg);
	if (message.length() < 1)
		return 0;
	if (message.length() > 255)
		message = message.substr(0, 255);

	std::shared_ptr<JoinedClient> client = popClientLua(L);
	client->sendChat(message);

	return 0;
}

static int LUA_getNumClients(lua_State* L)
{
	if (lua_gettop(L) != 0)
	{
		error("Expected 0 arguments getNumClients()");
		return 0;
	}

	if (!LUA_server)
	{
		error("Server not set");
		return 0;
	}

	lua_pushinteger(L, LUA_server->getNumClients());

	return 1;
}

static int LUA_getClientIdx(lua_State* L)
{
	scope("(LUA) getClientIdx");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getClientIdx(index)");
		return 0;
	}

	if (!LUA_server)
	{
		error("Server not set");
		return 0;
	}

	int idx = lua_tointeger(L, -1);

	if (idx < 0 || idx >= LUA_server->getNumClients())
	{
		error("Invalid client index passed, size: " + std::to_string(LUA_server->getNumClients()) + ", index: " + std::to_string(idx));
		return 0;
	}

	pushClientLua(L, LUA_server->getClientByIndex(idx));

	return 1;
}

static int LUA_clientGetName(lua_State* L)
{
	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument client:getName()");
		return 0;
	}

	std::shared_ptr<JoinedClient> client = popClientLua(L);
	lua_pushstring(L, client->name.c_str());

	return 1;
}

static int LUA_clientGetIP(lua_State* L)
{
	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument client:getIP()");
		return 0;
	}

	std::shared_ptr<JoinedClient> client = popClientLua(L);
	lua_pushstring(L, client->getIP().c_str());

	return 1;
}

static int LUA_clientGetID(lua_State* L)
{
	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument client:getID()");
		return 0;
	}

	std::shared_ptr<JoinedClient> client = popClientLua(L);
	lua_pushinteger(L, client->getNetId());

	return 1;
}

static int LUA_clientIsAdmin(lua_State* L)
{
	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument client:isAdmin()");
		return 0;
	}

	std::shared_ptr<JoinedClient> client = popClientLua(L);
	lua_pushboolean(L, client->isAdmin);

	return 1;
}

static int LUA_clientGiveControl(lua_State* L)
{
	if (lua_gettop(L) != 2)
	{
		error("Expected 2 arguments client:giveControl(dynamic)");
		return 0;
	}

	std::shared_ptr<Dynamic> player = LUA_pd->dynamics->popLua(L);

	if (!player)
	{
		error("Invalid dynamic passed to client:giveControl");
		return 0;
	}

	std::shared_ptr<JoinedClient> jc = popClientLua(L);

	if (!jc)
	{
		error("Invalid client object A passed to client:giveControl");
		return 0;
	}

	std::shared_ptr<ClientData> client = LUA_pd->getClient(jc);

	if (!client)
	{
		error("Invalid client object B passed to client:giveControl");
		return 0;
	}

	client->controlledObjects.push_back(player);

	return 0;
}

static int LUA_clientRemoveControl(lua_State* L)
{
	if (lua_gettop(L) != 2)
	{
		error("Expected 2 arguments client:removeControl(dynamic)");
		return 0;
	}

	std::shared_ptr<Dynamic> player = LUA_pd->dynamics->popLua(L);

	if (!player)
	{
		error("Invalid dynamic passed to client:removeControl");
		return 0;
	}

	std::shared_ptr<JoinedClient> jc = popClientLua(L);

	if (!jc)
	{
		error("Invalid client object A passed to client:removeControl");
		return 0;
	}

	std::shared_ptr<ClientData> client = LUA_pd->getClient(jc);

	if (!client)
	{
		error("Invalid client object B passed to client:removeControl");
		return 0;
	}

  	client->controlledObjects.erase(std::remove(client->controlledObjects.begin(), client->controlledObjects.end(), player), client->controlledObjects.end());

	return 0;
}

static int LUA_clientGetNumControlled(lua_State* L)
{
	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument client:getNumControlled()");
		return 0;
	}

	std::shared_ptr<JoinedClient> jc = popClientLua(L);

	if (!jc)
	{
		error("Invalid client object A passed to client:getNumControlled");
		return 0;
	}

	std::shared_ptr<ClientData> client = LUA_pd->getClient(jc);

	if (!client)
	{
		error("Invalid client object B passed to client:getNumControlled");
		return 0;
	}

	lua_pushinteger(L, client->controlledObjects.size());

	return 1;
}


static int LUA_clientGetControlledIdx(lua_State* L)
{
	if (lua_gettop(L) != 2)
	{
		error("Expected 1 argument client:getControlledIdx(index)");
		return 0;
	}

	int index = lua_tointeger(L,-1);
	lua_pop(L,1);

	std::shared_ptr<JoinedClient> jc = popClientLua(L);

	if (!jc)
	{
		error("Invalid client object A passed to client:getControlledIdx");
		return 0;
	}

	std::shared_ptr<ClientData> client = LUA_pd->getClient(jc);

	if (!client)
	{
		error("Invalid client object B passed to client:getControlledIdx");
		return 0;
	}

	if(index < 0 || index >= client->controlledObjects.size())
	{
		error("Index " + std::to_string(index) + " out of bounds for client:getControlledIdx");
		return 0;
	}
	
	LUA_pd->dynamics->pushLua(L, client->controlledObjects[index]);

	return 1;
}

void registerClientFunctions(lua_State* L)
{
	//Register client global functions:
	lua_register(L, "getNumClients", LUA_getNumClients);
	lua_register(L, "getClientIdx", LUA_getClientIdx);

	luaL_Reg regs[] = {
		{ "message", LUA_clientMessage},
		{ "kick", LUA_clientKick },
		{ "getName", LUA_clientGetName },
		{ "getIP", LUA_clientGetIP },
		{ "getID", LUA_clientGetID },
		{ "isAdmin", LUA_clientIsAdmin },
		{ "addControl", LUA_clientAddControl },
		{ "removeControl", LUA_clientRemoveControl },
		{ "getControlledIdx", LUA_clientGetControlledIdx },
		{ "getNumControlled", LUA_clientGetNumControlled },
		{ NULL, NULL }
	};

	luaL_newmetatable(L, "metatable_client");
	luaL_setfuncs(L, regs, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	lua_setglobal(L, "metatable_client");
}
