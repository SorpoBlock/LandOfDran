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

/*
	1 byte		-		packet type
	4 bytes		-		controlled dynamic id
	1 byte		-		movement control flags
	4 bytes		-		camera x direction
	4 bytes		-		camera y direction
	4 bytes		-		camera z direction
*/
inline ENetPacket* makeMovementInputs(netIDType controlledDynamicID, bool jump,bool forward,bool backward,bool left,bool right, glm::vec3 cameraDirection)
{
	ENetPacket* ret = enet_packet_create(NULL, 18, getFlagsFromChannel(Unreliable));

	unsigned char movementFlags = 0;
	movementFlags |= (jump ? 1 : 0);
	movementFlags |= (forward ? 2 : 0);
	movementFlags |= (backward ? 4 : 0);
	movementFlags |= (left ? 8 : 0); 
	movementFlags |= (right ? 16 : 0);

	ret->data[0] = (unsigned char)MovementInputs;
	memcpy(ret->data + 1, &controlledDynamicID, sizeof(netIDType));
	ret->data[1 + sizeof(netIDType)] = movementFlags;
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 0, &cameraDirection.x, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 1, &cameraDirection.y, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 2, &cameraDirection.z, sizeof(float));

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

inline ENetPacket *evalCommand(const std::string& password, const std::string &command)
{
	ENetPacket* ret = enet_packet_create(NULL, command.length() + password.length() + 4, getFlagsFromChannel(OtherReliable));
	ret->data[0] = (unsigned char)EvalCommand;

	unsigned short commandLength = command.length();
	memcpy(ret->data + 1, &commandLength, sizeof(unsigned short));

	memcpy(ret->data + 3, command.c_str(), command.length());
	ret->data[command.length() + 3] = (unsigned char)password.length();
	memcpy(ret->data + command.length() + 4, password.c_str(), password.length());
	//TODO: Hash password

	return ret;
}
