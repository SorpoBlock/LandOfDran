#include "Client.h"

void Client::tryApplyHeldPackets(const ClientProgramData& pd, const ExecutableArguments& cmdArgs)
{
	auto iter = packets.begin();
	while (iter != packets.end())
	{
		HeldServerPacket* tmp = *iter;
		if (tmp->applyPacket(pd,cmdArgs))
		{
			delete tmp;
			tmp = 0;
			iter = packets.erase(iter);
		}
		else
			++iter;
	}
}

void Client::run(const ClientProgramData& pd, const ExecutableArguments& cmdArgs)
{
	scope("Client::run");

	if (!valid)
		return;

	tryApplyHeldPackets(pd,cmdArgs);

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
				info("Lost connection with server!");
				return;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				//Each packet type gets its own file with the function that processes it
				FromServerPacketType type = (FromServerPacketType)event.packet->data[0];
				switch (type)
				{
					case AcceptConnection:
						packets.push_back(new AcceptConnectionPacket(packetHoldTime, event.packet));
						return;

					//Can't process packet
					case InvalidServer:
					default:
						error("Invalid packet type " + std::to_string(type) + " received from server.");
						enet_packet_destroy(event.packet);
						return;
				}

				//Packet will be destroyed in destructor of HeldServerPacket
				return;
			}
			case ENET_EVENT_TYPE_CONNECT:
			{
				error("Got some kind of double connection from server!");
				return;
			}
		}
	}
}

Client::Client(std::string ip,unsigned int port,unsigned int _packetHoldTime) : packetHoldTime(_packetHoldTime)
{
	scope("Client::Client");

	client = enet_host_create(NULL, 1, EndOfChannels, 0, 0);
	if (!client)
	{
		error("enet_host_create failed");
		return;
	}

	ENetAddress address;
	enet_address_set_host(&address, ip.c_str());
	address.port = port;

	peer = enet_host_connect(client, &address, EndOfChannels, 0);
	if (!peer)
	{
		error("enet_host_connect failed");
		return;
	}

	ENetEvent event;
	if (enet_host_service(client, &event, 5000) > 0 && ENET_EVENT_TYPE_CONNECT)
	{
		info("Connection made!");
		valid = true;
		return;
	}
	else
	{
		error("Could not connect!");
		valid = false; 
		return;
	}
}

Client::~Client()
{
	if (valid)
	{
		enet_peer_disconnect(peer,0);
		enet_host_destroy(client);
	}
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

void Client::send(ENetPacket *packet, PacketChannel channel)
{
	if (enet_peer_send(peer, channel, packet) < 0)
	{
		scope("JoinedClient::send");
		error("enet_peer_send failed");
	}
}
