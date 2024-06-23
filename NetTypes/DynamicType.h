#pragma once

#include "../LandOfDran.h"

#include "../Graphics/Mesh.h"

#include "NetType.h"

/*
	Objects that can move around the scene, includes projectiles
	Holds at least a reference to a 3d Model that has visual mesh(es) and a collision box
	ItemType and PlayerType are derived from DynamicType
*/
class DynamicType : public NetType
{
	//DynamicType should create and own its Model
	std::shared_ptr<Model> model = nullptr; 

	public:

	//Net types as opposed to SimObjects always just get their own packet
	//This is the packet from createTypePacket
	virtual void loadFromPacket(ENetPacket const* const packet) override;

	//Create the packet on the server to be passed to loadFromPacket on the client
	virtual ENetPacket* createTypePacket() const override;

	//Doesn't allocate anything, wait for loadFromPacket or server-side construction from lua
	DynamicType();

	//Deallocates model if it exists
	~DynamicType();
};



