#include "OtherFunctions.h"
#include "../GameLoop/ServerProgramData.h"

extern ServerProgramData* LUA_pd;

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
			//Bricks have no ptr field, so check for them before the SimObject handling below
			lua_getfield(L, a, "type");
			bool isBrick = lua_isinteger(L, -1) && lua_tointeger(L, -1) == BrickTypeId;
			lua_pop(L, 1);

			if (isBrick)
			{
				lua_getfield(L, a, "id");
				ret += "[Brick " + std::to_string(lua_tointeger(L, -1)) + "]";
				lua_pop(L, 1);
				continue;
			}

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

						case ClientTypeId:
						{
							lua_getfield(L, a, "id");
							int id = lua_tointeger(L, -1);
							lua_pop(L, 1); //id

							ret += "[Client " + std::to_string(id) + "]";
							break;
						}

						case StaticTypeId:
						{
							lua_getfield(L, a, "id");
							int id = lua_tointeger(L, -1);
							lua_pop(L, 1); //id

							ret += "[Static " + std::to_string(id) + "]";
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

//For setters that take exactly one number, logs an error and returns false for anything else
static bool getOnlyNumberArgument(lua_State* L, double& result)
{
	int args = lua_gettop(L);
	bool valid = args == 1 && lua_isnumber(L, 1);

	if (valid)
		result = lua_tonumber(L, 1);
	else
		error("Expected 1 number argument");

	lua_pop(L, args);
	return valid;
}

static int LUA_setTimeOfDay(lua_State* L)
{
	scope("LUA_setTimeOfDay");

	double fraction;
	if (!getOnlyNumberArgument(L, fraction))
		return 0;

	fraction -= floor(fraction);
	LUA_pd->worldTimeSeconds = fraction * DAY_LENGTH_SECONDS;
	LUA_pd->worldStateChanged = true;

	return 0;
}

static int LUA_getTimeOfDay(lua_State* L)
{
	double fraction = fmod(LUA_pd->worldTimeSeconds, DAY_LENGTH_SECONDS) / DAY_LENGTH_SECONDS;
	if (fraction < 0)
		fraction += 1.0;

	lua_pushnumber(L, fraction);
	return 1;
}

static int LUA_setTimeScale(lua_State* L)
{
	scope("LUA_setTimeScale");

	double scale;
	if (!getOnlyNumberArgument(L, scale))
		return 0;

	LUA_pd->timeScale = (float)scale;
	LUA_pd->worldStateChanged = true;

	return 0;
}

static int LUA_getTimeScale(lua_State* L)
{
	lua_pushnumber(L, LUA_pd->timeScale);
	return 1;
}

static int LUA_setWaterLevel(lua_State* L)
{
	scope("LUA_setWaterLevel");

	int args = lua_gettop(L);

	if (args == 0 || (args == 1 && lua_isnil(L, 1)))
		LUA_pd->waterEnabled = false;
	else if (args == 1 && lua_isnumber(L, 1))
	{
		LUA_pd->waterEnabled = true;
		LUA_pd->waterLevel = (float)lua_tonumber(L, 1);
	}
	else
	{
		error("Expected no arguments or 1 number argument");
		lua_pop(L, args);
		return 0;
	}

	lua_pop(L, args);
	LUA_pd->worldStateChanged = true;

	return 0;
}

static int LUA_getWaterLevel(lua_State* L)
{
	if (LUA_pd->waterEnabled)
		lua_pushnumber(L, LUA_pd->waterLevel);
	else
		lua_pushnil(L);

	return 1;
}

void registerOtherFunctions(lua_State* L)
{
	lua_register(L, "info", LUA_info);
	lua_register(L, "error", LUA_error);
	lua_register(L, "debug", LUA_debug);
	lua_register(L, "shutdown", LUA_shutdown);
	lua_register(L, "setTimeOfDay", LUA_setTimeOfDay);
	lua_register(L, "getTimeOfDay", LUA_getTimeOfDay);
	lua_register(L, "setTimeScale", LUA_setTimeScale);
	lua_register(L, "getTimeScale", LUA_getTimeScale);
	lua_register(L, "setWaterLevel", LUA_setWaterLevel);
	lua_register(L, "getWaterLevel", LUA_getWaterLevel);
}
