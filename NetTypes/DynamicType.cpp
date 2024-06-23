#include "DynamicType.h"

//Net types as opposed to SimObjects always just get their own packet
//This is the packet from createTypePacket
void DynamicType::loadFromPacket(ENetPacket const* const packet)
{

}

//Create the packet on the server to be passed to loadFromPacket on the client
ENetPacket* DynamicType::createTypePacket() const
{
	return nullptr; //make it
}

//Doesn't allocate anything, wait for loadFromPacket or server-side construction from lua
DynamicType::DynamicType()
{

}

//Deallocates model if it exists
DynamicType::~DynamicType()
{

}