#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

/*
	Opens the wrench dialog for a brick, see Interface/WrenchDialog.h
*/
class OpenWrenchDialogPacket : public HeldServerPacket
{
public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	OpenWrenchDialogPacket(unsigned int holdTime, ENetPacket* _packet);
	~OpenWrenchDialogPacket();
};
