#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

/*
	Reads a SoundLocationKind byte and what follows it, advancing byteIterator
	Returns false if the packet is too short or the kind is invalid, sets waiting instead if it names a Dynamic we don't have yet
*/
bool readSoundLocation(const ENetPacket* packet, unsigned int& byteIterator, Simulation& simulation, SoundLocation& result, bool& waiting);

class OneShotSoundPacket : public HeldServerPacket
{
public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	OneShotSoundPacket(unsigned int holdTime, ENetPacket* _packet);
	~OneShotSoundPacket();
};
