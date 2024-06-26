#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from DynamicType::createTypePacket on the server
*/
struct UpdateSimObjectsPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	UpdateSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet);
	~UpdateSimObjectsPacket();
};



