#include "Client.h"

void Client::run()
{
	if (!valid)
		return;

	ENetEvent event;
	int ret = enet_host_service(client, &event, 0);

	if (ret > 0)
	{
		switch (event.type)
		{
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				std::cout << "Disconnect!\n";
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				std::cout << "Receive!\n";
				break;
			}
			case ENET_EVENT_TYPE_CONNECT:
			{
				std::cout << "Connect?\n";
				break;
			}
		}
	}
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

void Client::testSend()
{
	ENetPacket* packet = enet_packet_create("packet", strlen("packet") + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet);
}
