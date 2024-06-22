#include "JoinedClient.h"

void JoinedClient::send(const char* data, unsigned int len, PacketChannel channel)
{
	ENetPacket* packet = enet_packet_create(data, len, getFlagsFromChannel(channel));
	if (!packet)
	{
		scope("JoinedClient::send");
		error("enet_packet_create failed");
		return;
	}
	if(enet_peer_send(peer, channel, packet) < 0)
	{
		scope("JoinedClient::send");
		error("enet_peer_send failed");
	}
}

void JoinedClient::dataReceived(ENetPacket* packet)
{
	info("Got packet!");
	std::cout << std::string((char*)packet->data, packet->dataLength) << "\n";
	send("Hey how's it going", 19, Unreliable);
}

JoinedClient::JoinedClient(ENetEvent& event,unsigned int _netID)
	: netID(_netID)
{
	char host[256];
	enet_address_get_host(&event.peer->address, host, 256);
	ip = std::string(host);
	info("Someone connected from " + ip);
	peer = event.peer;
	peer->data = this;
}

void JoinedClient::kick(KickReason reason)
{
	if (peer)
		enet_peer_disconnect(peer, reason);
	peer = nullptr;
}

JoinedClient::~JoinedClient()
{
	//Won't do anything if they disconnected already
	kick(KickReason::OtherReason);

	info("Client left");
}