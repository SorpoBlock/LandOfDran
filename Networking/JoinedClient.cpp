#include "JoinedClient.h"

void JoinedClient::dataReceived(ENetPacket* packet)
{
	info("Got packet!");
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