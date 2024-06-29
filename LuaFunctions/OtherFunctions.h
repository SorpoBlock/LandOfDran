#pragma once

/*
	Functions that are 'members' (in the loose Lua sense) of a SimObject class are handled in other files
	Functions *related* to SimObject classes are also there even if they aren't members
	Pushing and popping SimObjects would be handled in ObjHolder
	Schedule and Cancel are handled in LuaScheduler

	This is for random other global scope Lua functions that don't fit in the above categories
*/

#include "../LandOfDran.h"
#include "../Utility/ExecutableArguments.h"

extern ExecutableArguments* LUA_args;

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

//Logger wrapper functions
static int LUA_info(lua_State* L);
static int LUA_error(lua_State* L);
static int LUA_debug(lua_State* L);

static int LUA_shutdown(lua_State* L);

//Register all funcs in this file
void registerOtherFunctions(lua_State* L);