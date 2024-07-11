#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Lua on the server calling client:defaultMovementSettings(dynamic) for now
	That's a placeholder, basically a bunch of lua functions will set up the parameters for the player controller
*/
struct MovementSettingsPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	MovementSettingsPacket(unsigned int holdTime, ENetPacket* _packet);
	~MovementSettingsPacket();
};



