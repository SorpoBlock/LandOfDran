#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "../GameLoop/ServerProgramData.h"

extern ServerProgramData* LUA_pd;

//All of the actual functions are in ItemLua.cpp and static, they exist only to be called by Lua

/*
	Registers the item global functions, the item metatable, and the client functions for inventories
	Call after the dynamic and client metatables exist, items get a copy of every dynamic function
*/
void registerItemFunctions(lua_State* L);
