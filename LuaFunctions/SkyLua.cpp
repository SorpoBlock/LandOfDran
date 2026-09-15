#include "SkyLua.h"

#include <filesystem>

static ENetPacket* makeSkyboxPacket(const ServerProgramData* pd)
{
	std::vector<unsigned char> bytes;
	bytes.push_back(SkyboxPaths);
	for (const std::string& path : pd->skyboxPaths)
	{
		bytes.push_back((unsigned char)path.length());
		bytes.insert(bytes.end(), path.begin(), path.end());
	}
	return enet_packet_create(bytes.data(), bytes.size(), getFlagsFromChannel(OtherReliable));
}

void sendSkybox(const ServerProgramData* pd, JoinedClient* client)
{
	//Clients start out with the plain sky
	if (!pd->skyboxPaths[0].empty() || !pd->skyboxPaths[1].empty())
		client->send(makeSkyboxPacket(pd), OtherReliable);
}

//"" if this server has the skybox, otherwise what's wrong with it. Clients load their own copy from the same path
static std::string checkSkyboxPath(const std::string& path)
{
	if (path.length() > 255)
		return "the path is longer than 255 characters";

	if (!isPathInsideGameFolder(path))
		return "the path has to be relative and inside the game folder";

	if (lowercase(std::filesystem::path(path).extension().string()) == ".hdr")
		return std::filesystem::is_regular_file(path) ? "" : "the file doesn't exist";

	for (int a = 0; a < 5; a++)
	{
		std::string facePath = path + "_" + std::to_string(a) + ".png";
		if (!std::filesystem::is_regular_file(facePath))
			return facePath + " doesn't exist";
	}

	return "";
}

static int LUA_setSkybox(lua_State* L)
{
	scope("LUA_setSkybox");

	int args = lua_gettop(L);
	std::string paths[2];
	bool okay = args <= 2;
	for (int a = 0; a < args && okay; a++)
	{
		if (lua_isnil(L, a + 1))
			continue;

		if (lua_type(L, a + 1) != LUA_TSTRING)
			okay = false;
		else
			paths[a] = lua_tostring(L, a + 1);
	}
	lua_pop(L, args);

	if (!okay)
	{
		error("Expected setSkybox([day[, night]]), each a path or nil");
		return 0;
	}

	for (const std::string& path : paths)
	{
		if (path.empty())
			continue;

		std::string problem = checkSkyboxPath(path);
		if (!problem.empty())
		{
			error("Can't use skybox " + path + ": " + problem);
			return 0;
		}
	}

	LUA_pd->skyboxPaths[0] = paths[0];
	LUA_pd->skyboxPaths[1] = paths[1];
	LUA_server->broadcast(makeSkyboxPacket(LUA_pd), OtherReliable);

	return 0;
}

static int LUA_getSkybox(lua_State* L)
{
	lua_pop(L, lua_gettop(L));

	for (const std::string& path : LUA_pd->skyboxPaths)
	{
		if (path.empty())
			lua_pushnil(L);
		else
			lua_pushstring(L, path.c_str());
	}

	return 2;
}

void registerSkyFunctions(lua_State* L)
{
	lua_register(L, "setSkybox", LUA_setSkybox);
	lua_register(L, "getSkybox", LUA_getSkybox);
}
