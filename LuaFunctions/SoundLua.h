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

//For sounds the server plays on its own, like splashes: plays a registered sound once for everyone, silent if there's none by that name
void playSoundAt(const std::string& name, const glm::vec3& position, float pitch, float volume);

//Same, following a dynamic around while it plays
void playSoundOn(const std::string& name, const std::shared_ptr<Dynamic>& dynamic, float pitch, float volume);

//dynamic: and client: methods, registered in getDynamicFunctions and registerClientFunctions
int LUA_dynamicPlaySound(lua_State* L);
int LUA_dynamicStartSoundLoop(lua_State* L);
int LUA_clientPlaySound(lua_State* L);
int LUA_clientSetAudioEffect(lua_State* L);

//newSoundType, playSound, startSoundLoop, stopSoundLoop, setAudioEffect, setVoiceRange, getVoiceRange
void registerSoundFunctions(lua_State* L);
