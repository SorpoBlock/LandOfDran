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

//Every registered sound type, sent to a client right after their connection is accepted
void sendSoundTypes(const ServerProgramData* pd, JoinedClient* client);

//Loops that are playing and the reverb preset, sent once a client finishes loading
void sendSoundState(const ServerProgramData* pd, JoinedClient* client);

//dynamic: and client: methods, registered in getDynamicFunctions and registerClientFunctions
int LUA_dynamicPlaySound(lua_State* L);
int LUA_dynamicStartSoundLoop(lua_State* L);
int LUA_clientPlaySound(lua_State* L);
int LUA_clientSetAudioEffect(lua_State* L);

//newSoundType, playSound, startSoundLoop, stopSoundLoop, setAudioEffect
void registerSoundFunctions(lua_State* L);
