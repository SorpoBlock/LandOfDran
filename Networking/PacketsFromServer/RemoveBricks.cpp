#include "RemoveBricks.h"

bool RemoveBricksPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame || !simulation.bricks)
		return false;

	if (packet->dataLength < 3)
		return true;

	uint16_t count;
	memcpy(&count, packet->data + 1, sizeof(uint16_t));

	if (packet->dataLength < 3 + count * sizeof(netIDType))
	{
		error("RemoveBricks packet shorter than its brick count");
		return true;
	}

	for (unsigned int a = 0; a < count; a++)
	{
		netIDType id;
		memcpy(&id, packet->data + 3 + a * sizeof(netIDType), sizeof(netIDType));

		//Can be a brick we never received, if it was removed right after being added
		if (Brick* brick = simulation.bricks->find(id))
			simulation.bricks->remove(brick);
	}

	return true;
}

RemoveBricksPacket::RemoveBricksPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

RemoveBricksPacket::~RemoveBricksPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
