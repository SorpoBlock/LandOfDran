#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Item::makeStatePacket on the server
*/
struct ItemStatePacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	ItemStatePacket(unsigned int holdTime, ENetPacket* _packet);
	~ItemStatePacket();
};
