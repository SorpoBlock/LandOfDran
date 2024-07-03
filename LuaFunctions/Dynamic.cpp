#include "Dynamic.h"

ServerProgramData* LUA_pd = nullptr;

static int LUA_createDynamic(lua_State* L)
{
	scope("(LUA) createDynamic");

	int args = lua_gettop(L);

	if(args != 4)
	{
		error("Expected 4 arguments createDynamic(typeID, x, y, z)");
		return 0;
	}

	int typeID = lua_tointeger(L, 1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float z = lua_tonumber(L, 4);

	lua_pop(L, 4);

	if(!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	if (typeID >= LUA_pd->dynamicTypes.size() || typeID < 0)
	{
		error("typeID out of range");
		return 0;
	}

	std::shared_ptr<DynamicType> type = LUA_pd->dynamicTypes[typeID];
	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->create(type, btVector3(x, y, z));

	LUA_pd->dynamics->pushLua(L, dynamic);
	return 1;
}

static int LUA_destroyDynamic(lua_State* L)
{
	scope("(LUA) dynamic:destroy");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 argument dynamic:destroy()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	LUA_pd->dynamics->destroy(dynamic);

	return 0;
}

static int LUA_dynamicSetVelocity(lua_State* L)
{
	scope("(LUA) dynamic:setVelocity");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setVelocity(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->setVelocity(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetVelocity(lua_State* L)
{
	scope("(LUA) dynamic:getVelocity");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getVelocity()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btVector3 vel = dynamic->getVelocity();
	lua_pushnumber(L, vel.x());
	lua_pushnumber(L, vel.y());
	lua_pushnumber(L, vel.z());

	return 3;
}

static int LUA_dynamicSetPosition(lua_State* L)
{
	scope("(LUA) dynamic:setPosition");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setPosition(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->setPosition(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicActivate(lua_State* L)
{
	scope("(LUA) dynamic:activate");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 argument dynamic:activate()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetPosition(lua_State* L)
{
	scope("(LUA) dynamic:getPosition");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getPosition()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btVector3 vel = dynamic->getPosition();
	lua_pushnumber(L, vel.x());
	lua_pushnumber(L, vel.y());
	lua_pushnumber(L, vel.z());

	return 3;
}

static int LUA_getDynamicId(lua_State* L)
{
	scope("(LUA) getDynamicId");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getDynamicId(netId)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	int id = lua_tointeger(L, -1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->find(id);

	if (!dynamic)
	{
		error("Invalid dynamic object id passed, does it exist?");
		return 0;
	}

	LUA_pd->dynamics->pushLua(L, dynamic);

	return 1;
}

static int LUA_getDynamicIdx(lua_State* L)
{
	scope("(LUA) getDynamicIdx");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getDynamicIdx(index)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	int idx = lua_tointeger(L, -1);

	if(idx < 0 || idx >= LUA_pd->dynamics->size())
	{
		error("Invalid dynamic object index passed, size: " + std::to_string(LUA_pd->dynamics->size()) + ", index: " + std::to_string(idx));
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->get(idx);

	LUA_pd->dynamics->pushLua(L, dynamic);

	return 1;
}

static int getNumDynamics(lua_State* L)
{
	scope("(LUA) getNumDynamics");

	int args = lua_gettop(L);

	if (args != 0)
	{
		error("Expected 0 arguments getNumDynamics()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	lua_pushinteger(L, LUA_pd->dynamics->size());

	return 1;
}

luaL_Reg* getDynamicFunctions(lua_State *L)
{
	//Register dynamic global functions:
	lua_register(L, "createDynamic", LUA_createDynamic);
	lua_register(L, "getDynamicId", LUA_getDynamicId);
	lua_register(L, "getDynamicIdx", LUA_getDynamicIdx);
	lua_register(L, "getNumDynamics", getNumDynamics);

	//Create table of dynamic metatable functions:
	luaL_Reg* regs = new luaL_Reg[7];

	int iter = 0;
	regs[iter++] = { "destroy",     LUA_destroyDynamic };
	regs[iter++] = { "getVelocity", LUA_dynamicGetVelocity };
	regs[iter++] = { "getPosition", LUA_dynamicGetPosition };
	regs[iter++] = { "setPosition", LUA_dynamicSetPosition };
	regs[iter++] = { "setVelocity", LUA_dynamicSetVelocity };
	regs[iter++] = { "activate",    LUA_dynamicActivate };
	regs[iter++] = { NULL, NULL };

	return regs;
}
