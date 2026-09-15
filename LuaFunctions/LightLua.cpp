#include "LightLua.h"

#include <cmath>

//Reads count numbers starting at stack index first into out, logging usage and returning false if any is missing or not a finite number
static bool readNumbers(lua_State* L, int first, int count, float* out, const std::string& usage)
{
	for (int a = 0; a < count; a++)
	{
		if (!lua_isnumber(L, first + a))
		{
			error("Argument " + std::to_string(a + 1) + " isn't a number: " + usage);
			return false;
		}

		out[a] = (float)lua_tonumber(L, first + a);
		if (!std::isfinite(out[a]))
		{
			error("Argument " + std::to_string(a + 1) + " isn't a finite number: " + usage);
			return false;
		}
	}

	return true;
}

/*
	For light: methods, expects the light plus exactly count numbers, which are read into out
	Returns nullptr after logging why if anything is wrong, and leaves the stack empty either way
*/
static std::shared_ptr<Light> popLightAndNumbers(lua_State* L, int count, float* out, const std::string& usage)
{
	if (lua_gettop(L) != count + 1)
	{
		error("Expected " + std::to_string(count + 1) + " arguments " + usage);
		lua_settop(L, 0);
		return nullptr;
	}

	if (!LUA_pd->lights)
	{
		error("lights ObjHolder is null");
		lua_settop(L, 0);
		return nullptr;
	}

	if (!readNumbers(L, 2, count, out, usage))
	{
		lua_settop(L, 0);
		return nullptr;
	}

	lua_settop(L, 1);
	std::shared_ptr<Light> light = LUA_pd->lights->popLua(L);
	lua_settop(L, 0);

	if (!light)
		error("Invalid light passed, was it deleted already?");

	return light;
}

static int LUA_lightDestroy(lua_State* L)
{
	scope("(LUA) light:destroy");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:destroy()");
	if (light)
		LUA_pd->lights->destroy(light);

	return 0;
}

static int LUA_lightGetPosition(lua_State* L)
{
	scope("(LUA) light:getPosition");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getPosition()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getPosition().x);
	lua_pushnumber(L, light->getPosition().y);
	lua_pushnumber(L, light->getPosition().z);
	return 3;
}

static int LUA_lightSetPosition(lua_State* L)
{
	scope("(LUA) light:setPosition");

	float v[3];
	std::shared_ptr<Light> light = popLightAndNumbers(L, 3, v, "light:setPosition(x, y, z)");
	if (light)
		light->setPosition(glm::vec3(v[0], v[1], v[2]));

	return 0;
}

static int LUA_lightGetColor(lua_State* L)
{
	scope("(LUA) light:getColor");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getColor()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getColor().r);
	lua_pushnumber(L, light->getColor().g);
	lua_pushnumber(L, light->getColor().b);
	return 3;
}

static int LUA_lightSetColor(lua_State* L)
{
	scope("(LUA) light:setColor");

	float v[3];
	std::shared_ptr<Light> light = popLightAndNumbers(L, 3, v, "light:setColor(r, g, b)");
	if (light)
		light->setColor(glm::vec3(v[0], v[1], v[2]));

	return 0;
}

static int LUA_lightGetBrightness(lua_State* L)
{
	scope("(LUA) light:getBrightness");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getBrightness()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getBrightness());
	return 1;
}

static int LUA_lightSetBrightness(lua_State* L)
{
	scope("(LUA) light:setBrightness");

	float v;
	std::shared_ptr<Light> light = popLightAndNumbers(L, 1, &v, "light:setBrightness(brightness)");
	if (light)
		light->setBrightness(v);

	return 0;
}

static int LUA_lightGetFlicker(lua_State* L)
{
	scope("(LUA) light:getFlicker");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getFlicker()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getFlicker());
	return 1;
}

static int LUA_lightSetFlicker(lua_State* L)
{
	scope("(LUA) light:setFlicker");

	float v;
	std::shared_ptr<Light> light = popLightAndNumbers(L, 1, &v, "light:setFlicker(distance)");
	if (light)
		light->setFlicker(v);

	return 0;
}

static int LUA_lightGetBlink(lua_State* L)
{
	scope("(LUA) light:getBlink");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getBlink()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getBlinkSpeed());
	lua_pushnumber(L, light->getBlinkStrength());
	return 2;
}

static int LUA_lightSetBlink(lua_State* L)
{
	scope("(LUA) light:setBlink");

	float v[2];
	std::shared_ptr<Light> light = popLightAndNumbers(L, 2, v, "light:setBlink(speed, strength)");
	if (light)
		light->setBlink(v[0], v[1]);

	return 0;
}

static int LUA_lightGetCoronaWidth(lua_State* L)
{
	scope("(LUA) light:getCoronaWidth");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getCoronaWidth()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getCoronaWidth());
	return 1;
}

static int LUA_lightSetCoronaWidth(lua_State* L)
{
	scope("(LUA) light:setCoronaWidth");

	float v;
	std::shared_ptr<Light> light = popLightAndNumbers(L, 1, &v, "light:setCoronaWidth(width)");
	if (light)
		light->setCoronaWidth(v);

	return 0;
}

static int LUA_lightGetRange(lua_State* L)
{
	scope("(LUA) light:getRange");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getRange()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getRange());
	return 1;
}

static int LUA_lightGetDirection(lua_State* L)
{
	scope("(LUA) light:getDirection");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getDirection()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getDirection().x);
	lua_pushnumber(L, light->getDirection().y);
	lua_pushnumber(L, light->getDirection().z);
	return 3;
}

static int LUA_lightSetDirection(lua_State* L)
{
	scope("(LUA) light:setDirection");

	float v[3];
	std::shared_ptr<Light> light = popLightAndNumbers(L, 3, v, "light:setDirection(x, y, z)");
	if (light && !light->setDirection(glm::vec3(v[0], v[1], v[2])))
		error("light:setDirection needs a direction that isn't 0, 0, 0");

	return 0;
}

static int LUA_lightGetConeAngle(lua_State* L)
{
	scope("(LUA) light:getConeAngle");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getConeAngle()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getConeAngle());
	return 1;
}

static int LUA_lightSetConeAngle(lua_State* L)
{
	scope("(LUA) light:setConeAngle");

	float v;
	std::shared_ptr<Light> light = popLightAndNumbers(L, 1, &v, "light:setConeAngle(degrees)");
	if (light)
		light->setConeAngle(v);

	return 0;
}

static int LUA_lightGetSpin(lua_State* L)
{
	scope("(LUA) light:getSpin");

	std::shared_ptr<Light> light = popLightAndNumbers(L, 0, nullptr, "light:getSpin()");
	if (!light)
		return 0;

	lua_pushnumber(L, light->getSpin());
	return 1;
}

static int LUA_lightSetSpin(lua_State* L)
{
	scope("(LUA) light:setSpin");

	float v;
	std::shared_ptr<Light> light = popLightAndNumbers(L, 1, &v, "light:setSpin(degreesPerSecond)");
	if (light)
		light->setSpin(v);

	return 0;
}

static int LUA_createLight(lua_State* L)
{
	scope("(LUA) createLight");

	const std::string usage = "createLight(x, y, z, r, g, b, brightness, flicker, coronaWidth)";

	if (lua_gettop(L) != 9)
	{
		error("Expected 9 arguments " + usage);
		lua_settop(L, 0);
		return 0;
	}

	float v[9];
	bool valid = readNumbers(L, 1, 9, v, usage);
	lua_settop(L, 0);
	if (!valid)
		return 0;

	if (!LUA_pd->lights)
	{
		error("lights ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Light> light = LUA_pd->lights->create(glm::vec3(v[0], v[1], v[2]), glm::vec3(v[3], v[4], v[5]), v[6], v[7], v[8]);
	LUA_pd->lights->pushLua(L, light);
	return 1;
}

static int LUA_getLightId(lua_State* L)
{
	scope("(LUA) getLightId");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument getLightId(netId)");
		lua_settop(L, 0);
		return 0;
	}

	netIDType id = (netIDType)lua_tointeger(L, 1);
	lua_settop(L, 0);

	if (!LUA_pd->lights)
	{
		error("lights ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Light> light = LUA_pd->lights->find(id);
	if (!light)
	{
		error("Invalid light id passed, does it exist?");
		return 0;
	}

	LUA_pd->lights->pushLua(L, light);
	return 1;
}

static int LUA_getLightIdx(lua_State* L)
{
	scope("(LUA) getLightIdx");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument getLightIdx(index)");
		lua_settop(L, 0);
		return 0;
	}

	lua_Integer idx = lua_tointeger(L, 1);
	lua_settop(L, 0);

	if (!LUA_pd->lights)
	{
		error("lights ObjHolder is null");
		return 0;
	}

	if (idx < 0 || idx >= (lua_Integer)LUA_pd->lights->size())
	{
		error("Invalid light index passed, size: " + std::to_string(LUA_pd->lights->size()) + ", index: " + std::to_string(idx));
		return 0;
	}

	LUA_pd->lights->pushLua(L, LUA_pd->lights->get(idx));
	return 1;
}

static int LUA_getNumLights(lua_State* L)
{
	scope("(LUA) getNumLights");

	if (lua_gettop(L) != 0)
	{
		error("Expected 0 arguments getNumLights()");
		lua_settop(L, 0);
		return 0;
	}

	if (!LUA_pd->lights)
	{
		error("lights ObjHolder is null");
		return 0;
	}

	lua_pushinteger(L, LUA_pd->lights->size());
	return 1;
}

luaL_Reg* getLightFunctions(lua_State* L)
{
	lua_register(L, "createLight", LUA_createLight);
	lua_register(L, "getLightId", LUA_getLightId);
	lua_register(L, "getLightIdx", LUA_getLightIdx);
	lua_register(L, "getNumLights", LUA_getNumLights);

	luaL_Reg* regs = new luaL_Reg[21];

	int iter = 0;
	regs[iter++] = { "destroy",			LUA_lightDestroy };
	regs[iter++] = { "getPosition",		LUA_lightGetPosition };
	regs[iter++] = { "setPosition",		LUA_lightSetPosition };
	regs[iter++] = { "getColor",		LUA_lightGetColor };
	regs[iter++] = { "setColor",		LUA_lightSetColor };
	regs[iter++] = { "getBrightness",	LUA_lightGetBrightness };
	regs[iter++] = { "setBrightness",	LUA_lightSetBrightness };
	regs[iter++] = { "getFlicker",		LUA_lightGetFlicker };
	regs[iter++] = { "setFlicker",		LUA_lightSetFlicker };
	regs[iter++] = { "getBlink",		LUA_lightGetBlink };
	regs[iter++] = { "setBlink",		LUA_lightSetBlink };
	regs[iter++] = { "getCoronaWidth",	LUA_lightGetCoronaWidth };
	regs[iter++] = { "setCoronaWidth",	LUA_lightSetCoronaWidth };
	regs[iter++] = { "getRange",		LUA_lightGetRange };
	regs[iter++] = { "getDirection",	LUA_lightGetDirection };
	regs[iter++] = { "setDirection",	LUA_lightSetDirection };
	regs[iter++] = { "getConeAngle",	LUA_lightGetConeAngle };
	regs[iter++] = { "setConeAngle",	LUA_lightSetConeAngle };
	regs[iter++] = { "getSpin",			LUA_lightGetSpin };
	regs[iter++] = { "setSpin",			LUA_lightSetSpin };
	regs[iter++] = { NULL, NULL };

	return regs;
}
