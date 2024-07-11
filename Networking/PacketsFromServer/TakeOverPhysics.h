#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Lua on the server calling client:setPlayer
	Tells the client that it can be authoritative over this objects transform as its controller
*/
struct TakeOverPhysicsPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	TakeOverPhysicsPacket(unsigned int holdTime, ENetPacket* _packet);
	~TakeOverPhysicsPacket();
};



