#pragma once

#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"

/*	
	Do not attempt to assign a handle to JoinedClient to other objects directly
	Grab a smart pointer from the server for this
	server->getClientByNetId(source->getNetId());
	Also do not attempt to delete the JoinedClient, just call kick
*/
void chatMessageSent(JoinedClient * source, Server const * const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	//Invalid empty packet
	if (packet->dataLength < 2)
		return;

	//What message would they like to send?
	unsigned int messageLength = packet->data[1];

	if (packet->dataLength < 2 + messageLength)
		return;

	std::string message = std::string((char*)packet->data + 2, messageLength);
	message = source->name + ": " + message;
	info(message.c_str());

	messageLength = message.length();
	if(messageLength > 255)
	{
		messageLength = 255;
		message = message.substr(0, 255);
	}

	//Send the message to all clients
	ENetPacket * ret = enet_packet_create(NULL, messageLength + 2, getFlagsFromChannel(OtherReliable));
	ret->data[0] = (unsigned char)ChatMessageFromServer;
	ret->data[1] = (unsigned char)messageLength;
	memcpy(ret->data + 2, message.c_str(), messageLength);
	server->broadcast(ret, OtherReliable);
}
