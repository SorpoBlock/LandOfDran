#include "Server.h"

size_t Server::getNumClients() const
{
	return clients.size();
}

std::shared_ptr<JoinedClient> Server::getClientByNetId(netIDType id) const
{
	for (unsigned int a = 0; a < clients.size(); a++)
	{
		if (clients[a]->getNetId() == id)
			return clients[a];
	}
	return nullptr;
}

std::shared_ptr<JoinedClient> Server::getClientByIndex(unsigned int idx) const
{
	if (idx >= clients.size())
		return nullptr;
	return clients[idx];
}

void Server::broadcast(const char* data, unsigned int len, PacketChannel channel) const
{
	ENetPacket* packet = enet_packet_create(data, len, getFlagsFromChannel(channel));
	if (!packet)
	{
		scope("Server::broadcast");
		error("enet_packet_create failed");
		return;
	}
	enet_host_broadcast(server, channel, packet);
}

void Server::switchPacketType(JoinedClient * source, ENetPacket* packet, const void* pd)
{
	FromClientPacketType type = (FromClientPacketType)packet->data[0];

	switch (type)
	{
		case ConnectionRequest:
		{
			applyConnectionRequest(source,this,packet,pd);
			return;
		}
		case LoadingFinished:
		{
			clientFinishedLoading(source, this, packet, pd);
			return;
		}

		case InvalidClient:
		default:
			return;
	}
}

void Server::run(const void* pd)
{
	scope("Server::run");

	if (!valid)
		return;

	ENetEvent netEvent;

	int enetValue = enet_host_service(server, &netEvent, 0);

	if (enetValue < -1)
		error("enet_host_service error");
	else if (enetValue == 0)
		return;
	
	switch (netEvent.type)
	{
		case ENET_EVENT_TYPE_CONNECT:
		{
			clients.push_back(std::make_shared<JoinedClient>(netEvent,lastNetID));
			lastNetID++;
			break;
		}
		case ENET_EVENT_TYPE_RECEIVE:
		{
			JoinedClient* client = (JoinedClient*)netEvent.peer->data;
			if (!client)
			{
				error("Got packet that had no JoinedClient pointer!");
				break;
			}

			switchPacketType(client,netEvent.packet,pd);
			enet_packet_destroy(netEvent.packet);

			break;
		}
		case ENET_EVENT_TYPE_DISCONNECT:
		{
			//We could just jump directly to the joined client with the data pointer but we'd need to iterate the vector anyway to remove it
			auto iter = clients.begin();
			while (iter != clients.end())
			{
				std::shared_ptr<JoinedClient> client = *iter;
				if (client.get() == (JoinedClient*)netEvent.peer->data)
				{
					client.reset();
					clients.erase(iter);
					break;
				}
				++iter;
			}
			break;
		}
		default:
			break;
	}
}

Server::Server(int port)
{
	scope("Server::Server");

	address.host = ENET_HOST_ANY;
	address.port = port;
	server = enet_host_create(&address, 32, EndOfChannels, 0, 0);

	if (!server)
	{
		error("Could not create Enet server!");
		return;
	}

	valid = true;
}

Server::~Server()
{
	for (unsigned int a = 0; a < clients.size(); a++)
		clients[a]->kick(KickReason::ServerShutdown);

	if (valid)
		enet_host_destroy(server);
}
