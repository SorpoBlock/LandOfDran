#include "MeshAppearance.h"

bool MeshAppearancePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (packet->dataLength < 2 + sizeof(netIDType) + sizeof(glm::vec4))
		return true;

	netIDType id;
	memcpy(&id, packet->data + 1, sizeof(netIDType));

	std::shared_ptr<Dynamic> dynamic = simulation.dynamics->find(id);
	if (!dynamic)
		return false;

	unsigned char meshIdx = packet->data[1 + sizeof(netIDType)];

	glm::vec4 color;
	memcpy(&color.r, packet->data + 2 + sizeof(netIDType) + sizeof(float) * 0, sizeof(float));
	memcpy(&color.g, packet->data + 2 + sizeof(netIDType) + sizeof(float) * 1, sizeof(float));
	memcpy(&color.b, packet->data + 2 + sizeof(netIDType) + sizeof(float) * 2, sizeof(float));
	memcpy(&color.a, packet->data + 2 + sizeof(netIDType) + sizeof(float) * 3, sizeof(float));

	dynamic->setMeshColor(meshIdx, color);

	return true;
}

MeshAppearancePacket::MeshAppearancePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

MeshAppearancePacket::~MeshAppearancePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
