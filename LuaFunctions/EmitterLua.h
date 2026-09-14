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

//Every particle and emitter type, sent to a client right after their connection is accepted
void sendParticleEmitterTypes(const ServerProgramData* pd, JoinedClient* client);

//For effects the server makes on its own, like splashes: an emitter of the named type, nullptr without logging anything if there's no such type
std::shared_ptr<Emitter> spawnEmitterAt(const std::string& typeName, const glm::vec3& position);

//The name of an emitter's type, "" if its type is gone
std::string getEmitterTypeName(const Emitter& emitter);

bool emitterTypeExists(const std::string& typeName);

/*
	Registers addParticleType, addEmitterType, addEmitter, getParticleTable, getEmitterTable, getEmitterId, getEmitterIdx, and getNumEmitters
	Returns a list to be passed to ObjHolder<Emitter>::makeLuaMetatable which then deletes the list
	This function needs to be updated with each emitter related function added to the Lua API
*/
luaL_Reg* getEmitterFunctions(lua_State* L);
