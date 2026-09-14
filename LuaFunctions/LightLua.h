#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "../Networking/Server.h"
#include "../Networking/ObjHolder.h"
#include "../GameLoop/ServerProgramData.h"

extern Server* LUA_server;
extern ServerProgramData* LUA_pd;

/*
	Registers createLight, getLightId, getLightIdx, and getNumLights
	Returns a list to be passed to ObjHolder<Light>::makeLuaMetatable which then deletes the list
	This function needs to be updated with each light related function added to the Lua API
*/
luaL_Reg* getLightFunctions(lua_State* L);
