#include "MeshDecal.h"

bool MeshDecalPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 3 + sizeof(netIDType))
		return true;

	netIDType id;
	memcpy(&id, packet->data + 1, sizeof(netIDType));

	unsigned char meshIdx = packet->data[1 + sizeof(netIDType)];
	unsigned int nameLength = packet->data[2 + sizeof(netIDType)];

	if (packet->dataLength < 3 + sizeof(netIDType) + nameLength)
		return true;

	std::shared_ptr<Dynamic> dynamic = simulation.dynamics->find(id);
	if (!dynamic)
		return false;

	//A face this game doesn't have is left off
	std::string decalName((char*)packet->data + 3 + sizeof(netIDType), nameLength);
	dynamic->setMeshDecal(meshIdx, pd.getFaceDecal(decalName));

	return true;
}

MeshDecalPacket::MeshDecalPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

MeshDecalPacket::~MeshDecalPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
