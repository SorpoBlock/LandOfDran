#pragma once

#include "../Networking/JoinedClient.h"
#include "../SimObjects/Dynamic.h"

//Basically JoinedClient is lower level and used by the server for networking, ClientData is used in server-side packet functions
//ClientData contains references to a JoinedClient but also anything else that client 'owns' like a player, a camera, bricks, etc.
struct ClientData
{
	//Smart pointer to give to owned objects that want to refer to this object, managed by ServerProgramData methods
	std::shared_ptr<ClientData> me;

	//Lower level networking stuff
	std::shared_ptr<JoinedClient> client;

	//Physics objects like the player that this client handles primary simulation of
	std::vector<std::shared_ptr<Dynamic>> controlledObjects;
};
