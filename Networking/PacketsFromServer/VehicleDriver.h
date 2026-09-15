#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Vehicle::makeDriverPacket on the server
*/
struct VehicleDriverPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	VehicleDriverPacket(unsigned int holdTime, ENetPacket* _packet);
	~VehicleDriverPacket();
};
