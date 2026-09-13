#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

class AddSoundTypePacket : public HeldServerPacket
{
public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	AddSoundTypePacket(unsigned int holdTime, ENetPacket* _packet);
	~AddSoundTypePacket();
};
