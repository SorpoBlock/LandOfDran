#pragma once

#include "../LandOfDran.h"
#include "JoinedClient.h"
#include "../GameLoop/ServerProgramData.h"

#include "PacketsFromClient/ConnectionRequest.h"

/*
	Handles networking for the server and keeps track of clients that have joined
*/
class Server
{
	ENetAddress address;
	ENetHost* server = nullptr;

	bool valid = false;

	//Last netID given to a JoinedClient
	unsigned int lastNetID = 0;

	//Each one represents a connected player
	std::vector<std::shared_ptr<JoinedClient>> clients;

public:

	//ID that increments each time a client joins, not associated with player land of dran account in any way
	//Can return nullptr
	std::shared_ptr<JoinedClient> getClientByNetId(unsigned int id);

	//clients vector index
	//Can return nullptr
	std::shared_ptr<JoinedClient> getClientByIndex(unsigned int idx);

	bool isValid() const { return valid; }

	//A big switch statement that routes incomg packet to the appropriate function in PacketsFromClient files depending on type
	void switchPacketType(JoinedClient const * const source,ENetPacket* packet, const ServerProgramData& pd);

	//Send something to all connected clients
	void broadcast(const char* data, unsigned int len,PacketChannel channel);

	void run(const ServerProgramData& pd);

	Server(int port);
	~Server();
};
