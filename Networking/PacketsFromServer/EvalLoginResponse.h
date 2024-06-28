#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Dynamic::addToUpdatePacket on the server
*/
struct EvalLoginResponsePacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	EvalLoginResponsePacket(unsigned int holdTime, ENetPacket* _packet);
	~EvalLoginResponsePacket();
};



