#include "AddSimObjects.h"

bool AddSimObjectsPacket::applyPacket(const ClientProgramData& pd, const ExecutableArguments& cmdArgs)
{
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
				netIDType id, type;
				memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);
				memcpy(&type, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);
				glm::vec3 pos;
				getPosition(packet->data + byteIterator, pos);
				byteIterator += PositionBytes;
				glm::quat rot;
				getQuaternion(packet->data + byteIterator, rot);
				byteIterator += QuaternionBytes;

				std::shared_ptr<DynamicType> foundType = nullptr;
				for (unsigned int i = 0; i < pd.simulation.dynamicTypes.size(); i++)
				{
					if (pd.simulation.dynamicTypes[i]->getID() == type)
					{
						foundType = pd.simulation.dynamicTypes[i];
						break;
					}
				}

				//TODO: Actually return false if we can't find a type with the type ID given
				if (!foundType)
				{
					error("Could not find dynamic type server requested for object creation.");
					continue;
				}

				pd.simulation.dynamics->clientSetNextId(id);
				pd.simulation.dynamics->create(foundType, btVector3(pos.x,pos.y,pos.z));

				if (byteIterator >= packet->dataLength)
					break;
			}
			break;
		}
	}

	return true;
}

AddSimObjectsPacket::AddSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

AddSimObjectsPacket::~AddSimObjectsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
