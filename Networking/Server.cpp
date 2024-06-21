#include "Server.h"

void Server::run()
{
	if (!valid)
		return;

	ENetEvent netEvent;

	int enetValue = enet_host_service(server, &netEvent, 0);
	if (enetValue < -1)
		error("enet_host_service error");
	else if (enetValue > 0)
	{
		switch (netEvent.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
		{
			char host[256];
			enet_address_get_host(&netEvent.peer->address, host, 256);
			info("Someone connected from " + std::string(host));
			break;
		}
		}
	}
}

Server::Server(int port)
{
	scope("Server::Server");

	address.host = ENET_HOST_ANY;
	address.port = port;
	server = enet_host_create(&address, 32, 2, 0, 0);

	if (!server)
	{
		error("Could not create Enet server!");
		return;
	}

	valid = true;
}

Server::~Server()
{
	if (valid)
		enet_host_destroy(server);
}
