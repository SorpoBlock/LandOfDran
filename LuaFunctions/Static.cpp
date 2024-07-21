#include "Static.h"

static int LUA_createStatic(lua_State* L)
{
	scope("(LUA) createStatic");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments createStatic(typeID, x, y, z)");
		return 0;
	}

	int typeID = lua_tointeger(L, 1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float z = lua_tonumber(L, 4);

	lua_pop(L, 4);

	if (!LUA_pd->statics)
	{
		error("statics ObjHolder is null");
		return 0;
	}

	if (typeID >= LUA_pd->dynamicTypes.size() || typeID < 0)
	{
		error("typeID out of range");
		return 0;
	}

	std::shared_ptr<DynamicType> type = LUA_pd->dynamicTypes[typeID];
	std::shared_ptr<StaticObject> staticObject = LUA_pd->statics->create(type, btVector3(x, y, z), btQuaternion::getIdentity());

	LUA_pd->statics->pushLua(L, staticObject);
	return 1;
}



static int LUA_getStaticId(lua_State* L)
{
	scope("(LUA) getStaticId");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getStaticId(netId)");
		return 0;
	}

	if (!LUA_pd->statics)
	{
		error("statics ObjHolder is null");
		return 0;
	}

	int id = lua_tointeger(L, -1);

	std::shared_ptr<StaticObject> staticObject = LUA_pd->statics->find(id);

	if (!staticObject)
	{
		error("Invalid static object id passed, does it exist?");
		return 0;
	}

	LUA_pd->statics->pushLua(L, staticObject);

	return 1;
}

static int LUA_getStaticIdx(lua_State* L)
{
	scope("(LUA) getStaticIdx");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getStaticIdx(index)");
		return 0;
	}

	if (!LUA_pd->statics)
	{
		error("statics ObjHolder is null");
		return 0;
	}

	int idx = lua_tointeger(L, -1);

	if (idx < 0 || idx >= LUA_pd->statics->size())
	{
		error("Invalid static object index passed, size: " + std::to_string(LUA_pd->statics->size()) + ", index: " + std::to_string(idx));
		return 0;
	}

	std::shared_ptr<StaticObject> staticObject = LUA_pd->statics->get(idx);

	LUA_pd->statics->pushLua(L, staticObject);

	return 1;
}

static int getNumStatics(lua_State* L)
{
	scope("(LUA) getNumStatics");

	int args = lua_gettop(L);

	if (args != 0)
	{
		error("Expected 0 arguments getNumStatics()");
		return 0;
	}

	if (!LUA_pd->statics)
	{
		error("statics ObjHolder is null");
		return 0;
	}

	lua_pushinteger(L, LUA_pd->statics->size());

	return 1;
}

static int LUA_staticDestroy(lua_State* L)
{
	scope("(LUA) static:destroy");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 argument static:destroy()");
		return 0;
	}

	if (!LUA_pd->statics)
	{
		error("static ObjHolder is null");
		return 0;
	}

	std::shared_ptr<StaticObject> staticObject = LUA_pd->statics->popLua(L);

	if (!staticObject)
	{
		error("Invalid static object passed, was it deleted already?");
		return 0;
	}

	LUA_pd->statics->destroy(staticObject);

	return 0;
}

luaL_Reg* getStaticFunctions(lua_State* L)
{
	//Register dynamic global functions:
	lua_register(L, "createStatic", LUA_createStatic);
	lua_register(L, "getStaticId", LUA_getStaticId);
	lua_register(L, "getStaticIdx", LUA_getStaticIdx);
	lua_register(L, "getNumStatics", getNumStatics);

	//Create table of dynamic metatable functions:
	luaL_Reg* regs = new luaL_Reg[2];

	int iter = 0;
	regs[iter++] = { "destroy",     LUA_staticDestroy };
	regs[iter++] = { NULL, NULL };

	return regs;
}
