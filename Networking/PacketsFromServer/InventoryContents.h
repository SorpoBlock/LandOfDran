#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from ClientData::sendInventory on the server
*/
struct InventoryContentsPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	InventoryContentsPacket(unsigned int holdTime, ENetPacket* _packet);
	~InventoryContentsPacket();
};
