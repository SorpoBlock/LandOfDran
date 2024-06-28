#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

/*
	Returns a list to be passed to ObjHolder<Dynamic>::makeLuaMetatable which then deletes the list
	This function needs to be updated with each dynamic related function added to the Lua API
*/
luaL_Reg *getDynamicFunctions();