#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

/*
	Whether Lua lets this client use jets and a flashlight, see client:setJetsEnabled and client:setFlashlightEnabled
*/
class PlayerAbilitiesPacket : public HeldServerPacket
{
public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	PlayerAbilitiesPacket(unsigned int holdTime, ENetPacket* _packet);
	~PlayerAbilitiesPacket();
};
