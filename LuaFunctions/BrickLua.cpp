#include "BrickLua.h"

#include "../Bricks/BrickSaves.h"

static unsigned char colorByte(double value)
{
	return (unsigned char)std::clamp(value * 255.0 + 0.5, 0.0, 255.0);
}

//Methods are called as brick:method(...), so the brick is always argument 1
static Brick* brickArgument(lua_State* L)
{
	lua_pushvalue(L, 1);
	return LUA_pd->bricks->popLua(L);
}

static int LUA_addBrick(lua_State* L)
{
	scope("(LUA) addBrick");

	int args = lua_gettop(L);

	if (args != 10 && args != 11)
	{
		error("Expected 10 or 11 arguments addBrick(x, y, z, width, height, length, r, g, b, a[, angleID])");
		lua_settop(L, 0);
		return 0;
	}

	int width = (int)lua_tonumber(L, 4);
	int height = (int)lua_tonumber(L, 5);
	int length = (int)lua_tonumber(L, 6);
	int angleID = args == 11 ? (int)lua_tonumber(L, 11) : 0;

	if (width < 1 || width > 255 || height < 1 || height > 255 || length < 1 || length > 255)
	{
		error("Brick width, height, and length must be between 1 and 255");
		lua_settop(L, 0);
		return 0;
	}

	if (angleID < 0 || angleID > 3)
	{
		error("Brick angleID must be between 0 and 3");
		lua_settop(L, 0);
		return 0;
	}

	Brick desc;
	desc.x = (int)floor(lua_tonumber(L, 1));
	desc.y = (int)floor(lua_tonumber(L, 2));
	desc.z = (int)floor(lua_tonumber(L, 3));
	desc.width = width;
	desc.height = height;
	desc.length = length;
	desc.color = glm::u8vec4(colorByte(lua_tonumber(L, 7)), colorByte(lua_tonumber(L, 8)), colorByte(lua_tonumber(L, 9)), colorByte(lua_tonumber(L, 10)));
	desc.angleID = angleID;

	lua_settop(L, 0);

	Brick* brick = LUA_pd->bricks->add(desc);
	if (brick)
		LUA_pd->bricks->pushLua(L, brick);
	else
		lua_pushnil(L);

	return 1;
}

static int LUA_getNumBricks(lua_State* L)
{
	lua_settop(L, 0);
	lua_pushinteger(L, LUA_pd->bricks->size());
	return 1;
}

static int LUA_getBrickIdx(lua_State* L)
{
	scope("(LUA) getBrickIdx");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument getBrickIdx(index)");
		lua_settop(L, 0);
		return 0;
	}

	lua_Integer index = lua_tointeger(L, 1);
	lua_settop(L, 0);

	if (index < 0 || index >= (lua_Integer)LUA_pd->bricks->size())
	{
		error("Brick index out of range");
		return 0;
	}

	LUA_pd->bricks->pushLua(L, LUA_pd->bricks->get(index));
	return 1;
}

static int LUA_getBrickId(lua_State* L)
{
	scope("(LUA) getBrickId");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument getBrickId(id)");
		lua_settop(L, 0);
		return 0;
	}

	Brick* brick = LUA_pd->bricks->find((netIDType)lua_tointeger(L, 1));
	lua_settop(L, 0);

	if (brick)
		LUA_pd->bricks->pushLua(L, brick);
	else
		lua_pushnil(L);

	return 1;
}

static int LUA_getBrickAt(lua_State* L)
{
	scope("(LUA) getBrickAt");

	if (lua_gettop(L) != 3)
	{
		error("Expected 3 arguments getBrickAt(x, y, z)");
		lua_settop(L, 0);
		return 0;
	}

	Brick* brick = LUA_pd->bricks->getAt((int)floor(lua_tonumber(L, 1)), (int)floor(lua_tonumber(L, 2)), (int)floor(lua_tonumber(L, 3)));
	lua_settop(L, 0);

	if (brick)
		LUA_pd->bricks->pushLua(L, brick);
	else
		lua_pushnil(L);

	return 1;
}

static int LUA_clearAllBricks(lua_State* L)
{
	lua_settop(L, 0);
	LUA_pd->bricks->clear();
	return 0;
}

//Save functions take a file name inside the Saves folder, logs an error and returns "" for anything else
static std::string saveFileArgument(lua_State* L, int index)
{
	const char* fileName = lua_tostring(L, index);
	std::string path = fileName ? getSavePath(fileName) : "";
	if (path.empty())
		error("Expected a save file name inside the Saves folder, with no folders in it");
	return path;
}

static int LUA_saveBuild(lua_State* L)
{
	scope("(LUA) saveBuild");

	int args = lua_gettop(L);
	if (args != 1 && args != 2)
	{
		error("Expected 1 or 2 arguments saveBuild(fileName[, omitOwnership])");
		lua_settop(L, 0);
		return 0;
	}

	std::string path = saveFileArgument(L, 1);
	bool omitOwnership = args == 2 && lua_toboolean(L, 2);
	lua_settop(L, 0);

	lua_pushboolean(L, !path.empty() && saveLodBuild(*LUA_pd->bricks, path, omitOwnership));
	return 1;
}

static int LUA_loadLodSave(lua_State* L)
{
	scope("(LUA) loadLodSave");

	int args = lua_gettop(L);
	if (args != 1 && args != 4)
	{
		error("Expected 1 or 4 arguments loadLodSave(fileName[, x, y, z])");
		lua_settop(L, 0);
		return 0;
	}

	std::string path = saveFileArgument(L, 1);
	int offsetX = args == 4 ? (int)floor(lua_tonumber(L, 2)) : 0;
	int offsetY = args == 4 ? (int)floor(lua_tonumber(L, 3)) : 0;
	int offsetZ = args == 4 ? (int)floor(lua_tonumber(L, 4)) : 0;
	lua_settop(L, 0);

	int loaded = path.empty() ? -1 : loadLodBuild(*LUA_pd->bricks, path, offsetX, offsetY, offsetZ);
	if (loaded < 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, loaded);
	return 1;
}

static int LUA_loadBlocklandSave(lua_State* L)
{
	scope("(LUA) loadBlocklandSave");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument loadBlocklandSave(fileName)");
		lua_settop(L, 0);
		return 0;
	}

	std::string path = saveFileArgument(L, 1);
	lua_settop(L, 0);

	int loaded = path.empty() ? -1 : loadBlocklandBuild(*LUA_pd->bricks, LUA_pd->brickTypes, path);
	if (loaded < 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, loaded);
	return 1;
}

static int LUA_brickGetPosition(lua_State* L)
{
	scope("(LUA) brick:getPosition");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:getPosition()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	lua_pushinteger(L, brick->x);
	lua_pushinteger(L, brick->y);
	lua_pushinteger(L, brick->z);
	return 3;
}

static int LUA_brickGetDimensions(lua_State* L)
{
	scope("(LUA) brick:getDimensions");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:getDimensions()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	lua_pushinteger(L, brick->width);
	lua_pushinteger(L, brick->height);
	lua_pushinteger(L, brick->length);
	return 3;
}

static int LUA_brickGetAngleID(lua_State* L)
{
	scope("(LUA) brick:getAngleID");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:getAngleID()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	lua_pushinteger(L, brick->angleID);
	return 1;
}

static int LUA_brickGetColor(lua_State* L)
{
	scope("(LUA) brick:getColor");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:getColor()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	for (int channel = 0; channel < 4; channel++)
		lua_pushnumber(L, brick->color[channel] / 255.0);
	return 4;
}

static int LUA_brickSetColor(lua_State* L)
{
	scope("(LUA) brick:setColor");

	if (lua_gettop(L) != 5)
	{
		error("Expected 5 arguments brick:setColor(r, g, b, a)");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	LUA_pd->bricks->setColor(brick, glm::u8vec4(colorByte(lua_tonumber(L, 2)), colorByte(lua_tonumber(L, 3)), colorByte(lua_tonumber(L, 4)), colorByte(lua_tonumber(L, 5))));
	return 0;
}

static int LUA_brickIsColliding(lua_State* L)
{
	scope("(LUA) brick:isColliding");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:isColliding()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	lua_pushboolean(L, brick->collides);
	return 1;
}

static int LUA_brickSetColliding(lua_State* L)
{
	scope("(LUA) brick:setColliding");

	if (lua_gettop(L) != 2)
	{
		error("Expected 2 arguments brick:setColliding(collides)");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	LUA_pd->bricks->setColliding(brick, lua_toboolean(L, 2));
	return 0;
}

static int LUA_brickGetOwner(lua_State* L)
{
	scope("(LUA) brick:getOwner");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:getOwner()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	lua_pushinteger(L, brick->ownerID);
	return 1;
}

static int LUA_brickGetName(lua_State* L)
{
	scope("(LUA) brick:getName");

	if (lua_gettop(L) != 1)
	{
		error("Expected 1 argument brick:getName()");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	lua_pushstring(L, brick->name.c_str());
	return 1;
}

static int LUA_brickSetName(lua_State* L)
{
	scope("(LUA) brick:setName");

	if (lua_gettop(L) != 2 || !lua_isstring(L, 2))
	{
		error("Expected 2 arguments brick:setName(name)");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	brick->name = lua_tostring(L, 2);
	return 0;
}

static int LUA_brickRemove(lua_State* L)
{
	scope("(LUA) brick:remove");

	int args = lua_gettop(L);
	if (args != 1 && args != 2)
	{
		error("Expected 1 or 2 arguments brick:remove([showEffect])");
		return 0;
	}

	Brick* brick = brickArgument(L);
	if (!brick)
		return 0;

	LUA_pd->bricks->remove(brick, args == 2 && lua_toboolean(L, 2));
	return 0;
}

luaL_Reg* getBrickFunctions(lua_State* L)
{
	lua_register(L, "addBrick", LUA_addBrick);
	lua_register(L, "getNumBricks", LUA_getNumBricks);
	lua_register(L, "getBrickIdx", LUA_getBrickIdx);
	lua_register(L, "getBrickId", LUA_getBrickId);
	lua_register(L, "getBrickAt", LUA_getBrickAt);
	lua_register(L, "clearAllBricks", LUA_clearAllBricks);
	lua_register(L, "saveBuild", LUA_saveBuild);
	lua_register(L, "loadLodSave", LUA_loadLodSave);
	lua_register(L, "loadBlocklandSave", LUA_loadBlocklandSave);

	luaL_Reg* methods = new luaL_Reg[13];
	methods[0] = { "getPosition", LUA_brickGetPosition };
	methods[1] = { "getDimensions", LUA_brickGetDimensions };
	methods[2] = { "getAngleID", LUA_brickGetAngleID };
	methods[3] = { "getColor", LUA_brickGetColor };
	methods[4] = { "setColor", LUA_brickSetColor };
	methods[5] = { "isColliding", LUA_brickIsColliding };
	methods[6] = { "setColliding", LUA_brickSetColliding };
	methods[7] = { "getOwner", LUA_brickGetOwner };
	methods[8] = { "getName", LUA_brickGetName };
	methods[9] = { "setName", LUA_brickSetName };
	methods[10] = { "remove", LUA_brickRemove };
	methods[11] = { NULL, NULL };
	methods[12] = { NULL, NULL };
	return methods;
}
