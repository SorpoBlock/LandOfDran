#include "AcceptConnection.h"

bool AcceptConnectionPacket::applyPacket(const ClientProgramData& pd, const ExecutableArguments& cmdArgs)
{
	//Invalid empty packet
	if (packet->dataLength < 2)
		return true;

	switch (packet->data[1])
	{
		case ConnectionWrongVersion:
		{
			error("Server had different version than us!");
			pd.serverBrowser->setConnectionNote("Server version mismatch");
			return true;
		}
		case ConnectionNameUsed:
		{
			error("Someone is already using the name we requested!");
			pd.serverBrowser->setConnectionNote("Name already in use!");
			return true;
		}
		case ConnectionOkay:
		{
			pd.serverBrowser->setConnectionNote("");
			info("Connection accepted!");
			//do something else
			return true;
		}
	}

	return true;
}

AcceptConnectionPacket::AcceptConnectionPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

AcceptConnectionPacket::~AcceptConnectionPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
