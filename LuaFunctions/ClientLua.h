#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "../Networking/Server.h"
#include "../NetTypes/NetType.h"

extern Server *LUA_server;

//TODO: Merge these with the ObjHolder functions that basically do the exact same thing
void pushClientLua(lua_State *L,std::shared_ptr<JoinedClient> client);
std::shared_ptr<JoinedClient> popClientLua(lua_State *L);

void registerClientFunctions(lua_State *L);