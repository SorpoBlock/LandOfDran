#pragma once

#include "../LandOfDran.h"
#include "JoinedClient.h"

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

	bool isValid() const { return valid; }

	//Send something to all connected clients
	void broadcast(const char* data, unsigned int len,PacketChannel channel);

	void run();

	Server(int port);
	~Server();
};
