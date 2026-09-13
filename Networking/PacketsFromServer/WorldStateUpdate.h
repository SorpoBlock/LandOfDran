#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

//Time of day, how fast it passes, and water level, see LoopServer::broadcastWorldState
class WorldStateUpdatePacket : public HeldServerPacket
{
	public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	WorldStateUpdatePacket(unsigned int holdTime, ENetPacket* _packet);
	~WorldStateUpdatePacket();
};
