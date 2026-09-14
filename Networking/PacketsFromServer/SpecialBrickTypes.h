#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

//The server's special brick type IDs and names, matched to our own types by name, see BrickHolder::sendSpecialTypes for the layout
class SpecialBrickTypesPacket : public HeldServerPacket
{
	public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	SpecialBrickTypesPacket(unsigned int holdTime, ENetPacket* _packet);
	~SpecialBrickTypesPacket();
};
