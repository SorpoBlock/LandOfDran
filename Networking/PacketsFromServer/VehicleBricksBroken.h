#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Vehicle::makeBricksBrokenPacket on the server, when Lua's radiusImpulse breaks bricks off a destructable vehicle
*/
struct VehicleBricksBrokenPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	VehicleBricksBrokenPacket(unsigned int holdTime, ENetPacket* _packet);
	~VehicleBricksBrokenPacket();
};
