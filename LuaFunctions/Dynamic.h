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

//All of the actual functions are in Dynamic.cpp and static, they exist only to be called by Lua

/*
	This also registers global functions
	Returns a list to be passed to ObjHolder<Dynamic>::makeLuaMetatable which then deletes the list
	This function needs to be updated with each dynamic related function added to the Lua API
*/
luaL_Reg *getDynamicFunctions(lua_State *L);

//Pushes the Dynamic/Static/Brick Lua wrapper for a raycast hit and returns true, or pushes nil and returns false
bool pushRaycastResult(lua_State* L, btRigidBody* result);

//Shared by raycast() and client:getCursorItem(): pushes what was hit followed by the hit position, surface normal, and distance from start
//Pushes just nil if nothing was hit. Returns how many values were pushed
int pushRaycastHit(lua_State* L, btRigidBody* result, const btVector3& start, const btVector3& hitPosition, const btVector3& hitNormal);