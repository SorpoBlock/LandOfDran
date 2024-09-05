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

//All of the actual functions are in StaticObject.cpp and static, they exist only to be called by Lua

/*
	This also registers global functions
	Returns a list to be passed to ObjHolder<StaticObject>::makeLuaMetatable which then deletes the list
	This function needs to be updated with each static-object related function added to the Lua API
*/
luaL_Reg* getStaticFunctions(lua_State* L);