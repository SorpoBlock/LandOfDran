#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

//The day and night skyboxes from Lua's setSkybox, see Graphics/Skybox.h
class SkyboxPathsPacket : public HeldServerPacket
{
public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	SkyboxPathsPacket(unsigned int holdTime, ENetPacket* _packet);
	~SkyboxPathsPacket();
};
