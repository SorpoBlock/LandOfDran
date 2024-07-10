#pragma once

#include "../LandOfDran.h"
#include "JoinedClient.h"
#include "../LuaFunctions/EventManager.h"
//#include "../GameLoop/ServerProgramData.h"

/*
	Handles networking for the server and keeps track of clients that have joined
*/
class Server
{
	ENetAddress address;
	ENetHost* server = nullptr;

	bool valid = false;

	//Last netID given to a JoinedClient
	netIDType lastNetID = 0;

	//Each one represents a connected player
	std::vector<std::shared_ptr<JoinedClient>> clients;

public:

	size_t getNumClients() const;

	//ID that increments each time a client joins, not associated with player land of dran account in any way
	//Can return nullptr
	std::shared_ptr<JoinedClient> getClientByNetId(netIDType id) const;

	//clients vector index
	//Can return nullptr
	std::shared_ptr<JoinedClient> getClientByIndex(unsigned int idx) const;

	bool isValid() const { return valid; }

	/*
		A big switch statement that routes incomg packet to the appropriate function in PacketsFromClient files depending on type
		pd is only void cause I wanted to avoid circular dependency as SPD includes ObjHolder which includes this
	*/
	void switchPacketType(JoinedClient * source,ENetPacket* packet, const void* pd);

	//Send something to all connected clients
	void broadcast(const char* data, unsigned int len, PacketChannel channel) const;
	void broadcast(ENetPacket* packet, PacketChannel channel) const;

	//Send a chat message to everyone!
	void broadcastChat(std::string message) const;

	//Send something from the logger to any client with admin
	void updateAdminConsoles(const loggerLine &message) const;
	
	//eventManager and luaState are just to call ClientLeave event
	//eventManager is included in pd as well for use in packet functions
	void run(const void* pd,lua_State *L, EventManager* eventManager);

	Server(int port);
	~Server();
};

void handleDisconnect(JoinedClient * client,Server * server, const void* pdv, lua_State* L, EventManager* eventManager);
void applyConnectionRequest(JoinedClient *  source, Server const* const server, ENetPacket const* const packet, const void* pdv);
void clientFinishedLoading(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv);
void chatMessageSent(JoinedClient * source, Server const* const server, ENetPacket const* const packet, const void* pdv);
void attemptEvalLogin(JoinedClient * source, Server const* const server, ENetPacket const* const packet, const void* pdv);
void parseEvalCommand(JoinedClient * source, Server const* const server, ENetPacket const* const packet, const void* pdv);