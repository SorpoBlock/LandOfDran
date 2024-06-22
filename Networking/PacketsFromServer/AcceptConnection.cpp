#include "AcceptConnection.h"

bool AcceptConnectionPacket::applyPacket(const ClientProgramData& pd, const ExecutableArguments& cmdArgs)
{
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
