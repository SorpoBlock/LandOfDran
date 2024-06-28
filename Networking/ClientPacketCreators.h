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

//One byte lets the server know we finished phase one loading
inline ENetPacket* makeLoadingFinished()
{
	char theSmallestPacketEver[1];
	theSmallestPacketEver[0] = LoadingFinished;
	ENetPacket* ret = enet_packet_create(theSmallestPacketEver, 1, getFlagsFromChannel(JoinNegotiation));
	return ret;
}

//Send a chat message to the server
inline ENetPacket* makeChatMessage(const std::string &message)
{
	ENetPacket* ret = enet_packet_create(NULL, message.length() + 2, getFlagsFromChannel(OtherReliable));

	ret->data[0] = (unsigned char)ChatMessage;
	ret->data[1] = (unsigned char)message.length();
	memcpy(ret->data + 2, message.c_str(), message.length());

	return ret;
}

//Try to log into the server's eval console
inline ENetPacket* attemptEvalLogin(const std::string &password)
{
	ENetPacket* ret = enet_packet_create(NULL, password.length() + 2, getFlagsFromChannel(OtherReliable));
	ret->data[0] = (unsigned char)EvalLogin;
	ret->data[1] = (unsigned char)password.length();
	memcpy(ret->data + 2, password.c_str(), password.length());

	return ret;
}
