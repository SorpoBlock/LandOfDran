#pragma once

#include "../LandOfDran.h"

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
};
