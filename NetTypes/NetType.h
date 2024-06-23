#pragma once

#include "../LandOfDran.h"

/*
	Types of SimObject, held by SimObject and NetType
*/
enum SimObjectType
{
	InvalidSimType = 0,			//Default?
	DynamicType = 1,			//Objects that can move around the scene, includes projectiles
	ItemType = 2,				//Derived from dynamic, can be picked up
	PlayerType = 3,				//Derived from dynamic, can be controlled
	BrickType = 4,				//Bricks
	StaticMeshType = 5,			//Objects that don't move each frame, like a brick, but they just have their own mesh
	LightType = 6,				//Lights can be freestanding or mounted to objects
	EmitterType = 7,			//Emitters can be freestanding or mounted to objects
	BrickVehicleType = 8		//Vehicles made from bricks, might not actually have NetType
};

/*
	Abstract base class for net types
	net types in turn describe types of SimObjects
*/
class NetType
{
	bool loaded = false; 

	SimObjectType type = InvalidSimType;

	public:

	//Net types as opposed to SimObjects always just get their own packet
	//This is the packet from createTypePacket
	virtual void loadFromPacket(ENetPacket const* const packet) = 0;

	//Create the packet on the server to be passed to loadFromPacket on the client
	virtual ENetPacket * createTypePacket() const = 0; 

	virtual ~NetType() {};
};

