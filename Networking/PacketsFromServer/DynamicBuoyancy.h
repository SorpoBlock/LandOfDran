#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Dynamic::makeBuoyancyPacket on the server, when Lua changes a dynamic's buoyancy
	Clients need it for the dynamics they simulate in water themselves, like their player
*/
struct DynamicBuoyancyPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	DynamicBuoyancyPacket(unsigned int holdTime, ENetPacket* _packet);
	~DynamicBuoyancyPacket();
};
