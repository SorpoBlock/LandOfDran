#include "WorldStateUpdate.h"

bool WorldStateUpdatePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 1 + sizeof(double) + sizeof(float) * 2 + 1)
		return true;

	enet_uint8* data = packet->data + 1;

	memcpy(&simulation.worldTimeSeconds, data, sizeof(double));
	data += sizeof(double);

	memcpy(&simulation.timeScale, data, sizeof(float));
	data += sizeof(float);

	memcpy(&simulation.waterLevel, data, sizeof(float));
	data += sizeof(float);

	simulation.waterEnabled = data[0] != 0;

	return true;
}

WorldStateUpdatePacket::WorldStateUpdatePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

WorldStateUpdatePacket::~WorldStateUpdatePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
