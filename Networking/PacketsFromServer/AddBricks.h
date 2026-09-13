#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

//Bricks the server added or changed, see BrickHolder::writeRecord for the layout
class AddBricksPacket : public HeldServerPacket
{
	public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	AddBricksPacket(unsigned int holdTime, ENetPacket* _packet);
	~AddBricksPacket();
};
