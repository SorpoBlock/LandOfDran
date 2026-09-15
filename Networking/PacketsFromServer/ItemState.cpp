#include "ItemState.h"

bool ItemStatePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 1 + sizeof(netIDType) + Item::stateBytes)
		return true;

	netIDType id;
	memcpy(&id, packet->data + 1, sizeof(netIDType));

	//An item that hasn't arrived yet doesn't need this, its creation packet is made later with its state as of then
	std::shared_ptr<Dynamic> dynamic = simulation.dynamics->find(id);
	if (!dynamic || dynamic->getKind() != DynamicKind_Item)
		return true;

	std::static_pointer_cast<Item>(dynamic)->readState(packet->data + 1 + sizeof(netIDType), false, simulation.idealBufferSize);
	return true;
}

ItemStatePacket::ItemStatePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

ItemStatePacket::~ItemStatePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
