#pragma once

#include "../LandOfDran.h"

class Server
{
	ENetAddress address;
	ENetHost* server = nullptr;

	bool valid = false;

public:

	bool isValid() const { return valid; }

	void run();

	Server(int port);
	~Server();
};
