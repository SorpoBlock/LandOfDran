#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "../LandOfDran.h"

/*
	Events are triggered by C++ code and then call all bound Lua functions
	ex. 'ClientJoined' or 'ClientLeft' or 'BrickClicked'
*/
struct LuaEvent
{
	//The name of the event itself i.e. 'ClientJoined'
	std::string name = "";
	//List of all the names of Lua functions that will be called
	std::vector<std::string> boundFunctions;
};

/*
	Server only
	Only one of these should exist
	Holds all events and calls them when they are triggered
	Add new events in constructor
*/
struct EventManager
{
	static std::vector<LuaEvent> events;

	//From c++, call all bound Lua functions for an event
	static void callEvent(lua_State* L,std::string eventName, int numValues);
	//From lua
	static int registerEventListener(lua_State* L);
	//From lua
	static int unregisterEventListener(lua_State *L);
	//How many listeners are bound to this event
	static int getNumListeners(lua_State* L);
	//Name of the listener at index
	static int getListenerIdx(lua_State* L);

	EventManager(lua_State* L);
	~EventManager();
};