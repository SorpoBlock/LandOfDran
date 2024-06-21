#include "Client.h"

void Client::run()
{
	if (!valid)
		return;
}

Client::Client()
{
	scope("Client::Client");

	client = enet_host_create(NULL, 1, 2, 0, 0);
	if (!client)
	{
		error("enet_host_create failed");
		return;
	}

	ENetAddress address;
	enet_address_set_host(&address, "localhost");
	address.port = DEFAULT_PORT;

	peer = enet_host_connect(client, &address, 2, 0);
	if (!peer)
	{
		error("enet_host_connect failed");
		return;
	}

	ENetEvent event;
	if (enet_host_service(client, &event, 5000) > 0 && ENET_EVENT_TYPE_CONNECT)
	{
		info("Connection made!");
	}
	else
	{
		error("Could not connect!");
	}

	valid = true;
}

Client::~Client()
{
	if(valid)
		enet_host_destroy(client);
}
