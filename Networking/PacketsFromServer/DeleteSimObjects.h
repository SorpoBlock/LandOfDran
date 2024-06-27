#pragma once

#include "../HeldServerPacket.h"

/*
	This packet is just a list of IDs of recently deleted objects of this type of SimObject
*/
struct DeleteSimObjectsPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	DeleteSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet);
	~DeleteSimObjectsPacket();
};




