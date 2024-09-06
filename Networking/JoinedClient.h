#pragma once

#include "../LandOfDran.h"

/*
	These are held by the Server and represent a handle to a single person who joined the server for the duration of their play
	Don't confuse with Client which is something the client program holds a single instance of
*/
class JoinedClient
{
	//Unique ID specific to clients, incremented each time one joins, used to associate enet events with a JoinedClient
	netIDType netID = -1;

	ENetPeer* peer = nullptr;

	std::string ip = "";

public:

	float getPing() const { return peer->roundTripTime; }

	//Set this to ClientData I guess
	void* userData = nullptr;

	//Points to itself, used to pass to lua functions pretty much
	//Derivitive of a shared_ptr the server holds for all clients
	std::shared_ptr<JoinedClient> me;

	//Did they log in with the eval password?
	bool isAdmin = false;

	std::string name = "";

	std::string getIP() const { return ip; }

	netIDType getNetId() const { return netID; }

	float getPacketLoss() { return peer->packetLoss; }

	//Send a packet to this client
	void send(const char* data, unsigned int len, PacketChannel channel) const;

	void send(ENetPacket* packet, PacketChannel channel) const;

	//Create a client from a connection event
	JoinedClient(ENetEvent& event,unsigned int _netID);

	void sendChat(std::string message) const;

	//Similar to the destructor, but can be called before it to specify a reason
	void kick(KickReason reason);

	/*
		Destroy and disconnect a client
		JoinedClients should have destruction managed by the Server through shared ptr
	*/
	~JoinedClient();
};
