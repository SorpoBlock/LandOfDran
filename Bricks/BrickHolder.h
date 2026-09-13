#pragma once

#include "Brick.h"
#include "../Physics/PhysicsWorld.h"
#include "../Networking/Server.h"
#include "../External/RTree.h"

#include <unordered_set>

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

class InstancedBrickRenderer;

/*
	Every brick in the world, with overlap checks, lookups, physics bodies, and (server side) networking
	Not an ObjHolder: bricks are far too numerous for a full SimObject each
*/
class BrickHolder
{
	std::vector<Brick*> bricks;
	std::unordered_map<netIDType, Brick*> byId;

	//Inclusive min/max voxel bounds of every brick
	RTree<Brick*, int, 3, float> tree;

	//Box shapes shared by every brick of the same footprint and height
	std::unordered_map<unsigned int, btBoxShape*> shapes;

	std::shared_ptr<PhysicsWorld> world = nullptr;

	//Non-owning, nullptr on the client
	Server* server = nullptr;

	//Non-owning, client only
	InstancedBrickRenderer* renderer = nullptr;

	//Server only: bricks added or changed since the last sendRecent, and IDs removed since then
	std::unordered_set<netIDType> pendingSends;
	std::vector<netIDType> pendingRemovals;

	//Server only: removed since the last sendRecent, and clients should show them popping loose
	std::vector<netIDType> pendingEffectRemovals;

	//Sends RemoveBricks packets for ids, each under the MTU
	void sendRemovals(const std::vector<netIDType>& ids, bool showEffect) const;

	netIDType lastNetId = 0;

	std::string metatableName = "";

	void insert(Brick* brick);
	void createBody(Brick* brick);
	void destroyBody(Brick* brick);

	//AddBricks packets for these bricks, each under the MTU
	std::vector<ENetPacket*> makeAddPackets(const std::vector<const Brick*>& toSend) const;

	public:

	//Bytes per brick in AddBricks packets
	static constexpr unsigned int recordBytes = 19;

	static void writeRecord(const Brick* brick, enet_uint8* data);
	static Brick readRecord(const enet_uint8* data);

	//Returns nullptr if the brick is zero sized, out of bounds, or would overlap another brick
	Brick* add(const Brick& desc);

	/*
		Client: applies a record from an AddBricks packet without validating it, the server already did
		A record for a brick we already have is an update to its color or collision
	*/
	Brick* addFromServer(const Brick& desc);

	/*
		Removes and deletes the brick
		showEffect has clients show it popping loose and flying off, for a brick removed on purpose one at a time
	*/
	void remove(Brick* brick, bool showEffect = false);

	void clear();

	void setColliding(Brick* brick, bool collides);
	void setColor(Brick* brick, const glm::u8vec4& color);

	//Would a brick with this min corner and rotated size overlap an existing one
	bool overlaps(int x, int y, int z, int footprintWidth, int height, int footprintLength) const;

	//Brick occupying this voxel, or nullptr
	Brick* getAt(int x, int y, int z) const;

	//nullptr if no brick has that ID
	Brick* find(netIDType netId) const;

	size_t size() const { return bricks.size(); }
	Brick* get(size_t index) const { return bricks[index]; }

	//Server: broadcast everything added, changed, or removed since the last call, once per tick
	void sendRecent();

	//Server: every brick, to a client that just finished loading
	void sendAll(JoinedClient const* client) const;

	/*
		Lua bricks are tables with integer id and type (BrickTypeId) fields, looked up by id on each use so a
		table for a removed brick safely resolves to nullptr
		makeLuaMetatable deletes functions, like ObjHolder::makeLuaMetatable
	*/
	void makeLuaMetatable(lua_State* L, const std::string& name, luaL_Reg* functions);
	void pushLua(lua_State* L, const Brick* brick) const;
	//Pops the table on top of the stack, nullptr (with an error logged) if it isn't an existing brick
	Brick* popLua(lua_State* L) const;

	//Client: every brick added, changed, or removed from here on is passed along to this renderer
	void setRenderer(InstancedBrickRenderer* _renderer) { renderer = _renderer; }

	//Pass nullptr as _server on the client
	BrickHolder(std::shared_ptr<PhysicsWorld> _world, Server* _server = nullptr);
	~BrickHolder();
};
