#pragma once

#include "../HeldServerPacket.h"

/*
	This packet processes the acceptance or rejection of a connection request by the server
	If accepted the server will also give some information about how much stuff needs to be loaded
*/
struct AcceptConnectionPacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	AcceptConnectionPacket(unsigned int holdTime, ENetPacket* _packet);
	~AcceptConnectionPacket();
};
