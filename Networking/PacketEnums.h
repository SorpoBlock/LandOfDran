#pragma once

#include "enet/enet.h"

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
	ObjectUpdates = 3,		//Unreliable sequenced: Used for SimObject updates, mainly snapshop interpolation
	Unreliable = 4,			//Unreliable unsequenced: Used for some minor effects like playing sounds
	EndOfChannels = 5		//Not a channel, used in enet_host_create
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
