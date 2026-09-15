#include "InventoryContents.h"

/*
	1 byte					-	packet type
	4 bytes per slot		-	net ID of the item in each inventory slot, NO_ID for an empty one
*/
bool InventoryContentsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 1 + sizeof(netIDType) * inventorySize)
		return true;

	//The item bar looks each one up as it's drawn, so items that haven't arrived yet show once they do
	for (int a = 0; a < inventorySize; a++)
		memcpy(&simulation.inventory[a], packet->data + 1 + sizeof(netIDType) * a, sizeof(netIDType));

	return true;
}

InventoryContentsPacket::InventoryContentsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

InventoryContentsPacket::~InventoryContentsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
