#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "../GameLoop/ServerProgramData.h"

extern ServerProgramData* LUA_pd;

/*
	Registers global brick functions
	Returns brick methods to be passed to BrickHolder::makeLuaMetatable, which then deletes the list
*/
luaL_Reg* getBrickFunctions(lua_State* L);
