#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from Dynamic::setMeshDecal on the server
*/
struct MeshDecalPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	MeshDecalPacket(unsigned int holdTime, ENetPacket* _packet);
	~MeshDecalPacket();
};
