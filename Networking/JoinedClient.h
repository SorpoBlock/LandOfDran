#pragma once

#include "../LandOfDran.h"

/*
	For use with enet_peer_disconnect 
*/
enum KickReason
{
	OtherReason = 1000,		//Given if the destructor is called on JoinedClient
	ServerShutdown = 1001	//Broadcast from the destructor of the Server itself
};

/*
	These are held by the server and represent a handle to a single person who joined the server
	Don't confuse with Client which is something the client program holds a single instance of
*/
class JoinedClient
{
	//Unique ID specific to clients, incremented each time one joins, used to associate enet events with a JoinedClient
	unsigned int netID = -1;

	ENetPeer* peer = nullptr;

	std::string ip = "";

public:

	unsigned int getTempID() const { return netID; }

	//Send a packet to this client
	void send(const char* data, unsigned int len, PacketChannel channel);

	//Called when data is received from this client
	void dataReceived(ENetPacket* packet);

	//Create a client from a connection event
	JoinedClient(ENetEvent& event,unsigned int _netID);

	//Similar to the destructor, but can be called before it to specify a reason
	void kick(KickReason reason);

	//Destroy and disconnect a client
	~JoinedClient();
};
