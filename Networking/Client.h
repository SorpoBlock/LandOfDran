#pragma once

#include "../LandOfDran.h"

/*
	This is the networking interface that the client program should have one instance of, that managers sending and receiving from the server
	Do not confuse with JoinedClient, which is something the server may manage one or more of
*/
class Client
{
	ENetHost* client = nullptr;
	ENetPeer* peer = nullptr;

	bool valid = false;

public:

	bool isValid() const { return valid; }

	void run();

	Client();
	~Client();

	void testSend();
};
