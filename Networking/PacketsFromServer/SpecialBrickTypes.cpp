#include "SpecialBrickTypes.h"

bool SpecialBrickTypesPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte  - packet type
		2 bytes - how many types follow
		For each:
		2 bytes - the server's type ID
		1 byte  - name length, then the name
	*/

	//Comes in before any bricks do, so this doesn't wait for InGame
	if (packet->dataLength < 3)
		return true;

	uint16_t count;
	memcpy(&count, packet->data + 1, sizeof(uint16_t));

	simulation.brickTypeToServer.resize(pd.brickTypes.getSpecialCount() + 1, 0);

	size_t byteIterator = 3;
	int missing = 0;
	std::string missingNames = "";

	for (unsigned int a = 0; a < count; a++)
	{
		if (packet->dataLength < byteIterator + 3)
			break;

		uint16_t serverID;
		memcpy(&serverID, packet->data + byteIterator, sizeof(uint16_t));
		unsigned int nameLength = packet->data[byteIterator + 2];
		byteIterator += 3;

		if (packet->dataLength < byteIterator + nameLength)
			break;

		std::string name((char*)packet->data + byteIterator, nameLength);
		byteIterator += nameLength;

		int local = pd.brickTypes.findSpecial(name);
		if (simulation.brickTypeFromServer.size() <= serverID)
			simulation.brickTypeFromServer.resize(serverID + 1, 0);
		simulation.brickTypeFromServer[serverID] = local < 0 ? 0 : (uint16_t)(local + 1);

		if (local < 0)
		{
			missing++;
			if (missing <= 10)
				missingNames += (missingNames.empty() ? "" : ", ") + name;
		}
		else
			simulation.brickTypeToServer[local + 1] = serverID;
	}

	if (missing > 0)
		error("The server has " + std::to_string(missing) + " special brick types we don't, they'll look like plain bricks: " + missingNames);

	return true;
}

SpecialBrickTypesPacket::SpecialBrickTypesPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

SpecialBrickTypesPacket::~SpecialBrickTypesPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
