#include "OtherFunctions.h"

ExecutableArguments* LUA_args = nullptr;

void stackDump(lua_State* L) 
{
	int i;
	int top = lua_gettop(L);
	for (i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(L, i);
		switch (t) {

		case LUA_TSTRING:  /* strings */
			printf("`%s'", lua_tostring(L, i));
			break;

		case LUA_TBOOLEAN:  /* booleans */
			printf(lua_toboolean(L, i) ? "true" : "false");
			break;

		case LUA_TNUMBER:  /* numbers */
			printf("%g", lua_tonumber(L, i));
			break;

		default:  /* other values */
			printf("%s", lua_typename(L, t));
			break;

		}
		printf("  ");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}

static int LUA_shutdown(lua_State* L)
{
	if (LUA_args)
	{
		info("shutdown() called");
		LUA_args->mainLoopRun = false;
	}
	else
		error("LUA_args not set");

	return 0;
}

//Common part of LUA_info/error/debug
std::string makeStringFromArgs(lua_State* L)
{
	int args = lua_gettop(L);

	if(args == 0)
	{
		return "[Empty line]";
	}

	std::string ret = "";

	for (unsigned int a = 1; a < args+1; a++)
	{
		if (lua_isstring(L, a))
		{
			const char *str = lua_tostring(L, a);
			if (str)
				ret += std::string(str);
			else
				ret += "[invalid string]";
			continue;
		}

		if (lua_isboolean(L, a))
		{
			ret += lua_toboolean(L, a) ? "true" : "false";
			continue;
		}

		if (lua_isnil(L, a))
		{
			ret += "[nil]";
			continue;
		}

		if (lua_isnumber(L, a))
		{
			ret += std::to_string(lua_tonumber(L, a));
			continue;
		}

		if (lua_istable(L, a))
		{
			lua_getfield(L, a, "ptr");
			if (lua_isuserdata(L, -1))
			{
				lua_pop(L, 1); //ptr

				lua_getfield(L, a, "type");
				if(lua_isinteger(L,-1))
				{
					SimObjectType type = (SimObjectType)lua_tointeger(L, -1);
					lua_pop(L, 1); //type
					switch (type)
					{
						case DynamicTypeId:
						{
							lua_getfield(L, a, "id");
							int id = lua_tointeger(L, -1);
							lua_pop(L, 1); //id

							ret += "[Dynamic " + std::to_string(id)+"]";
							break;
						}

						case InvalidSimTypeId:
						default:
							ret += "[Unknown Simobject]";
					}
					continue;
				}
				else
				{
					ret += "[Unknown Userdata]";
					lua_pop(L, 1); //type
					continue;
				}
			}
			else
				lua_pop(L, 1); //ptr


			ret += "[table]";
			continue;
		}

		if (lua_isfunction(L, a))
		{
			ret += "[function]";
			continue;
		}

		ret += "[unknown]";
	}

	lua_pop(L, args);

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
	lua_register(L, "shutdown", LUA_shutdown);
}
