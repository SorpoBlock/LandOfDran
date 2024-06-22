#include "ConnectionRequest.h"

void applyConnectionRequest(JoinedClient const* const source, ENetPacket const* const packet, const ServerProgramData& pd)
{
	std::cout << "Got a connection packet!!!: " << std::string((char*)packet->data, packet->dataLength) << "\n";
}

