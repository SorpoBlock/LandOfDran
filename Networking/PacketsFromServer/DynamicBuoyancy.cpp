#include "DynamicBuoyancy.h"

bool DynamicBuoyancyPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 1 + sizeof(netIDType) + sizeof(float))
		return true;

	netIDType id;
	memcpy(&id, packet->data + 1, sizeof(netIDType));

	std::shared_ptr<Dynamic> dynamic = simulation.dynamics->find(id);
	if (!dynamic)
		return false;

	memcpy(&dynamic->buoyancy, packet->data + 1 + sizeof(netIDType), sizeof(float));

	return true;
}

DynamicBuoyancyPacket::DynamicBuoyancyPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

DynamicBuoyancyPacket::~DynamicBuoyancyPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
