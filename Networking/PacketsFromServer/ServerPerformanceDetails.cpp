#include "ServerPerformanceDetails.h"

bool ServerPerformanceDetailsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	//Too short to have any data
	if (packet->dataLength < 9)
		return true;

	//Get slowest frame of the last second
	memcpy(&simulation.serverLastSlowestFrame, packet->data + 1, sizeof(float));
	memcpy(&simulation.serverAverageFrame, packet->data + 1 + sizeof(float), sizeof(float));

	return true;
}

ServerPerformanceDetailsPacket::ServerPerformanceDetailsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

ServerPerformanceDetailsPacket::~ServerPerformanceDetailsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
