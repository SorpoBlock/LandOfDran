#pragma once

#include "../LandOfDran.h"
#include "../GameLoop/ServerProgramData.h"

/*
	For use with enet_peer_disconnect 
*/
enum KickReason
{
	OtherReason = 1000,			//Given if the destructor is called on JoinedClient
	ServerShutdown = 1001,		//Broadcast from the destructor of the Server itself
	ConnectionRejected = 1002	//See the details of the AcceptConnection packet for more info
};

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

	std::string name = "";

	std::string getIP() const { return ip; }

	netIDType getNetId() const { return netID; }

	//Send a packet to this client
	void send(const char* data, unsigned int len, PacketChannel channel) const;

	void send(ENetPacket* packet, PacketChannel channel) const;

	//Create a client from a connection event
	JoinedClient(ENetEvent& event,unsigned int _netID);

	//Similar to the destructor, but can be called before it to specify a reason
	void kick(KickReason reason);

	/*
		Destroy and disconnect a client
		JoinedClients should have destruction managed by the Server through shared ptr
	*/
	~JoinedClient();
};
