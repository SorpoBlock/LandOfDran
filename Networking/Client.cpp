#include "Client.h"

void Client::run()
{
	scope("Client::run");

	if (!valid)
		return;

	ENetEvent event;
	int ret = enet_host_service(client, &event, 0);

	if (ret < 0)
	{
		error("enet_host_server failed");
		return;
	}

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
				std::cout << event.channelID << " channel received a packet\n";
				std::cout << std::string((char*)event.packet->data, event.packet->dataLength) << "\n";
				break;
			}
			case ENET_EVENT_TYPE_CONNECT:
			{
				error("Got some kind of double connection from server!");
				break;
			}
		}
	}
}

Client::Client()
{
	scope("Client::Client");

	client = enet_host_create(NULL, 1, EndOfChannels, 0, 0);
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

void Client::send(const char* data, unsigned int len, PacketChannel channel)
{
	ENetPacket* packet = enet_packet_create(data, len, getFlagsFromChannel(channel));
	if (!packet)
	{
		scope("Client::send");
		error("enet_packet_create failed");
		return;
	}
	if (enet_peer_send(peer, channel, packet) < 0)
	{
		scope("JoinedClient::send");
		error("enet_peer_send failed");
	}
}
