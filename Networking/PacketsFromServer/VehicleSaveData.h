#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from vehicleSaveRequest in Networking/PacketsFromClient/VehicleFiles.cpp
*/
struct VehicleSaveDataPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	VehicleSaveDataPacket(unsigned int holdTime, ENetPacket* _packet);
	~VehicleSaveDataPacket();
};
