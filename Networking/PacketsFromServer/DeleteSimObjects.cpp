#include "DeleteSimObjects.h"

bool DeleteSimObjectsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	//Too short to have any objects
	if (packet->dataLength < 4)
		return true;

	switch ((SimObjectType)packet->data[1])
	{
		case DynamicTypeId:
		{
			unsigned int numObjects = packet->data[2];
			for (unsigned int a = 0; a < numObjects; a++)
			{
				netIDType id;
				memcpy(&id, packet->data + 3 + a * sizeof(netIDType), sizeof(netIDType));

				simulation.dynamics->destroyByID(id);
			}
		}
	}

	return true;
}

DeleteSimObjectsPacket::DeleteSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

DeleteSimObjectsPacket::~DeleteSimObjectsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
