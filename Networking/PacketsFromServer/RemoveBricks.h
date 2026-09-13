#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

//u16 count followed by the netIDs of bricks the server removed
class RemoveBricksPacket : public HeldServerPacket
{
	public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	RemoveBricksPacket(unsigned int holdTime, ENetPacket* _packet);
	~RemoveBricksPacket();
};
