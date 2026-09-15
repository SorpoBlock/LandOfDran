#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from openVehicleWrenchDialog in LuaFunctions/VehicleLua.cpp
*/
struct OpenVehicleWrenchPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	OpenVehicleWrenchPacket(unsigned int holdTime, ENetPacket* _packet);
	~OpenVehicleWrenchPacket();
};
