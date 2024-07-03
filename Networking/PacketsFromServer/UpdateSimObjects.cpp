#include "UpdateSimObjects.h"

bool UpdateSimObjectsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	//TODO: Have server not send updates until we confirm we've loaded in
	if (cmdArgs.gameState != InGame)
		return true;

	//Too short to have any objects
	if (packet->dataLength < 4)
		return true;

	switch ((SimObjectType)packet->data[1])
	{
	case DynamicTypeId:
	{
		unsigned int numObjects = packet->data[2];
		unsigned int byteIterator = 3;
		for (unsigned int a = 0; a < numObjects; a++)
		{
			netIDType id;
			memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
			byteIterator += sizeof(netIDType);
			glm::vec3 pos;
			getPosition(packet->data + byteIterator, pos);
			byteIterator += PositionBytes;
			glm::quat rot;
			getQuaternion(packet->data + byteIterator, rot);
			byteIterator += QuaternionBytes;

			std::shared_ptr<Dynamic> toUpdate = simulation.dynamics->find(id);
			if (toUpdate)
			{
				toUpdate->interpolator.addSnapshot(pos, rot, simulation.idealBufferSize);
			}

			if (byteIterator >= packet->dataLength)
				break;
		}
		break;
	}
	}

	return true;
}

UpdateSimObjectsPacket::UpdateSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

UpdateSimObjectsPacket::~UpdateSimObjectsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
