#pragma once

#include "../Networking/JoinedClient.h"
#include "../SimObjects/Dynamic.h"

//Basically JoinedClient is lower level and used by the server for networking
//ClientData contains references to a JoinedClient but also anything else that client 'owns' like a player, a camera, bricks, etc.
struct ClientData
{
	std::shared_ptr<ClientData> me;
	std::shared_ptr<JoinedClient> client;
	std::shared_ptr<Dynamic> player;
};
