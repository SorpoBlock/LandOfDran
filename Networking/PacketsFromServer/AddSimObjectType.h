#pragma once

#include "../HeldServerPacket.h"

/*
	This packet comes from DynamicType::createTypePacket on the server
*/
struct AddSimObjectTypePacket : public HeldServerPacket
{
	//Returns true if the packet can be discarded (it was applied or it expired)
	virtual bool applyPacket(const ClientProgramData& pd, const ExecutableArguments& cmdArgs) override;

	AddSimObjectTypePacket(unsigned int holdTime, ENetPacket* _packet);
	~AddSimObjectTypePacket();
};



