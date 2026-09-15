#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Vehicle::makeBrickPackets on the server
*/
struct VehicleBricksPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	VehicleBricksPacket(unsigned int holdTime, ENetPacket* _packet);
	~VehicleBricksPacket();
};
