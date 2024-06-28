#pragma once

#include "enet/enet.h"

//Represents an ID sent from server to client for unambigous identification
//Every SimObject and SimObjectType should have one that's unique to it within its type
//Different types of objects can share the same ID
typedef unsigned int netIDType;

/*
	For use with enet_host_broadcast and enet_peer_send
	These channels should be the same between server and client
	Their values are arbitrary, it only matters that they're consistant
*/
enum PacketChannel
{
	JoinNegotiation = 0,	//Reliable: Used for authentication, and receiving data about SimObject types
	BrickLoading = 1,		//Reliable: Used just for loading the initial build, unfortunately no unsequenced reliable packets in Enet
	OtherReliable = 2,		//Reliable: Used for anything else that needs a relaible packet
	ObjectUpdates = 3,		//Unreliable sequenced: Used for SimObject updates, mainly snapshot interpolation
	Unreliable = 4,			//Unreliable unsequenced: Used for some minor effects like playing sounds
	EndOfChannels = 5		//Not a channel, used in enet_host_create and enet_host_connect
};

//Each PacketChannel has an ideal type of packet
inline enet_uint32 getFlagsFromChannel(PacketChannel channel)
{
	switch (channel)
	{
		case BrickLoading:
		case JoinNegotiation: 
		case OtherReliable:
			return ENET_PACKET_FLAG_RELIABLE;
		case ObjectUpdates:
			return 0;
		case Unreliable:
			return ENET_PACKET_FLAG_UNSEQUENCED;
		default:
			return 0;
	}
}

/*
	Up to 256 types of packet from client to server
*/
enum FromClientPacketType : unsigned char
{
	InvalidClient = 0,		//Default value
	ConnectionRequest = 1,	//Client wants to connect, includes their name and login token if not a guest
	LoadingFinished = 2,	//Let server know we loaded all types and can start receiving actual object updates now
	ChatMessage = 3,		//Send a chat message to the server
	EvalLogin = 4,			//Try to log into the server's eval console with admin password
};

/*
	Up to 256 types of packet from server to client
*/
enum FromServerPacketType : unsigned char
{
	InvalidServer = 0,		//Default value
	AcceptConnection = 1,	//Connection request accepted, say how many types we expect to load
	AddSimObjectType = 2,
	AddSimObjects = 3,
	UpdateSimObjects = 4,
	DeleteSimObjects = 5,
	ChatMessageFromServer = 6,
	EvalLoginResponse = 7	//Response to EvalLogin, did you get the password right?
};

//For use with AcceptConnection packets
enum ConnectionResponse	: unsigned char
{
	ConnectionOkay = 1,				//Accepted, includes info on amount of SimObjectTypes
	ConnectionWrongVersion = 2,		//Rejected, wrong game version
	ConnectionNameUsed = 3			//Rejected, someone already has your guest name
};
