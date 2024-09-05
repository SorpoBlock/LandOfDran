#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

class ServerPerformanceDetailsPacket : public HeldServerPacket
{
	public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	ServerPerformanceDetailsPacket(unsigned int holdTime, ENetPacket* _packet);
	~ServerPerformanceDetailsPacket();
};
