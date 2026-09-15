#pragma once

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "../Networking/Server.h"
#include "../GameLoop/ServerProgramData.h"

extern Server* LUA_server;
extern ServerProgramData* LUA_pd;

//The day and night skyboxes, sent once a client finishes loading
void sendSkybox(const ServerProgramData* pd, JoinedClient* client);

//setSkybox, getSkybox
void registerSkyFunctions(lua_State* L);
