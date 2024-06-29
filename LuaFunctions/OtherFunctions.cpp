#include "OtherFunctions.h"

//Common part of LUA_info/error/debug
std::string makeStringFromArgs(lua_State* L)
{
	int args = lua_gettop(L);

	if(args == 0)
	{
		return "[Empty line]";
	}

	std::string ret = "";

	for (unsigned int a = 0; a < args; a++)
	{
		if (lua_isstring(L, -1))
		{
			const char *str = lua_tostring(L, -1);
			if (str)
				ret += std::string(str);
			else
				ret += "[invalid string]";

			lua_pop(L, 1);
			continue;
		}

		if (lua_isboolean(L, -1))
		{
			ret += lua_toboolean(L, -1) ? "true" : "false";
			lua_pop(L, 1);
			continue;
		}

		if (lua_isnil(L, -1))
		{
			ret += "[nil]";
			lua_pop(L, 1);
			continue;
		}

		if (lua_isnumber(L, -1))
		{
			ret += std::to_string(lua_tonumber(L, -1));
			lua_pop(L, 1);
			continue;
		}

		if (lua_istable(L, -1))
		{
			ret += "[table]";
			lua_pop(L, 1);
			continue;
		}

		if (lua_isfunction(L, -1))
		{
			ret += "[function]";
			lua_pop(L, 1);
			continue;
		}

		ret += "[unknown]";
		lua_pop(L, 1);
	}

	return ret;
}

static int LUA_info(lua_State* L)
{
	scope("LUA_info");

	std::string line = makeStringFromArgs(L);
	info(line);

	return 0;
}

static int LUA_error(lua_State* L)
{
	scope("LUA_error");

	std::string line = makeStringFromArgs(L);
	error(line);

	return 0;
}

static int LUA_debug(lua_State* L)
{
	scope("LUA_debug");

	std::string line = makeStringFromArgs(L);
	debug(line);

	return 0;
}

void registerOtherFunctions(lua_State* L)
{
	lua_register(L, "info", LUA_info);
	lua_register(L, "error", LUA_error);
	lua_register(L, "debug", LUA_debug);
}
