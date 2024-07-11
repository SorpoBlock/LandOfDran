#pragma once

#include "../../LandOfDran.h"
#include "../HeldServerPacket.h"

/*
	1 byte - packet type
	1 byte - camera flags
		0b0001 - bound to object
		0b0010 - lock camera direction, no effect if bound to obj
		0b0100 - lock camera position, no effect if bound to obj
		0b1000 - lock up vector, different behavior if bound to obj
	If bound to object:
	4 bytes - object ID
	4 bytes - max follow distance
	If cam dir locked:
	4 bytes - x (direction)
	4 bytes - y
	4 bytes - z
	If cam pos locked:
	4 bytes - x (position)
	4 bytes - y
	4 bytes - z
	If camera up vector not locked and camera not bound to object:
	4 bytes - x (up vector)
	4 bytes - y
	4 bytes - z
*/
class CameraSettingsPacket : public HeldServerPacket
{
public:

	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs) override;

	CameraSettingsPacket(unsigned int holdTime, ENetPacket* _packet);
	~CameraSettingsPacket();
};


