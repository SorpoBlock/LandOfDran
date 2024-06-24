#include "AddSimObjectType.h"

bool AddSimObjectTypePacket::applyPacket(const ClientProgramData& pd, const ExecutableArguments& cmdArgs)
{
	info("packet!");

	/*
		1 byte  - type of packet (AddSimObjectType)
		1 byte  - type of SimObject type (dynamic, brick, emitter...)
		4 bytes - ID of type
		From here it's specific to the kind of SimObject
	*/

	//Too short to even specify any type specific data
	if (packet->dataLength < 6)
		return true;

	switch ((SimObjectType)packet->data[1])
	{
		case DynamicTypeId:
		{
			auto newType = std::make_shared<DynamicType>();
			newType->loadFromPacket(packet);
			pd.getSimulation()->dynamicTypes.push_back(newType);
			break;
		}

		case InvalidSimTypeId:
		default:
			error("Invalid SimObject type for AddSimObjectTypePacket");
			return true;
	}

	return true;
}

AddSimObjectTypePacket::AddSimObjectTypePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

AddSimObjectTypePacket::~AddSimObjectTypePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
