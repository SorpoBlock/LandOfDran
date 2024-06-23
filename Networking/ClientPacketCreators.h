#pragma once

#include "../LandOfDran.h"

/*
	Global inline functions that help create miscellaneous packet types from passed parameters
	These are for packets sent to the server from the client
*/

/*
	1 byte		-	packet type
	1 byte		-	client game version
	1 byte		-	name length, max 255
	1-255 bytes	-	name
*/
inline ENetPacket* makeConnectionRequest(std::string name)
{
	ENetPacket* ret = enet_packet_create(NULL, name.length() + 3, getFlagsFromChannel(JoinNegotiation));

	ret->data[0] = (unsigned char)ConnectionRequest;
	ret->data[1] = (unsigned char)GAME_VERSION;
	ret->data[2] = (unsigned char)name.length();
	memcpy(ret->data + 3, name.c_str(), name.length());

	return ret;
}
