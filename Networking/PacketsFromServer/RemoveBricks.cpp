#include "RemoveBricks.h"

bool RemoveBricksPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame || !simulation.bricks)
		return false;

	//Packet type, u16 count, then whether to show the bricks popping loose
	static constexpr unsigned int headerBytes = 4;

	if (packet->dataLength < headerBytes)
		return true;

	uint16_t count;
	memcpy(&count, packet->data + 1, sizeof(uint16_t));
	bool showEffect = packet->data[3] & 1;

	if (packet->dataLength < headerBytes + count * sizeof(netIDType))
	{
		error("RemoveBricks packet shorter than its brick count");
		return true;
	}

	for (unsigned int a = 0; a < count; a++)
	{
		netIDType id;
		memcpy(&id, packet->data + headerBytes + a * sizeof(netIDType), sizeof(netIDType));

		//Can be a brick we never received, if it was removed right after being added
		Brick* brick = simulation.bricks->find(id);
		if (!brick)
			continue;

		if (showEffect && simulation.brickDebris)
		{
			simulation.brickDebris->spawn(*brick);
			pd.audio->playSound("BrickBreak", SoundLocation::at(brick->getWorldCenter()));
		}

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
