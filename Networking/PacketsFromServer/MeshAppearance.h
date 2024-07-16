#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Dynamic::addToUpdatePacket on the server
*/
struct MeshAppearancePacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	MeshAppearancePacket(unsigned int holdTime, ENetPacket* _packet);
	~MeshAppearancePacket();
};




