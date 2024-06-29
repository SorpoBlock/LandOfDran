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

luaL_Reg* getDynamicFunctions(lua_State *L)
{
	//Register dynamic global functions:
	lua_register(L, "createDynamic", LUA_createDynamic);

	//Create table of dynamic metatable functions:
	luaL_Reg* regs = new luaL_Reg[2];

	int iter = 0;
	regs[iter++] = { "destroy", LUA_destroyDynamic };
	regs[iter++] = { NULL, NULL };

	return regs;
}
