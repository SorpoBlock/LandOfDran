#pragma once

#include "../LandOfDran.h"

/*
	Types of SimObject, held by SimObject and NetType
*/
enum SimObjectType : unsigned char //might be used for packets who knows
{
	InvalidSimTypeId = 0,			//Default?
	DynamicTypeId = 1,				//Objects that can move around the scene, includes projectiles
	ItemTypeId = 2,					//Derived from dynamic, can be picked up
	PlayerTypeId = 3,				//Derived from dynamic, can be controlled
	BrickTypeId = 4,				//Bricks
	StaticMeshTypeId = 5,			//Objects that don't move each frame, like a brick, but they just have their own mesh
	LightTypeId = 6,				//Lights can be freestanding or mounted to objects
	EmitterTypeId = 7,				//Emitters can be freestanding or mounted to objects
	BrickVehicleTypeId = 8			//Vehicles made from bricks, might not actually have NetType
};

/*
	Abstract base class for net types
	net types in turn describe types of SimObjects
*/
class NetType
{
	protected:

	bool loaded = false; 

	SimObjectType type = InvalidSimTypeId;

	netIDType id = -1; //Default value suppresses warning

	public:

	//Net types as opposed to SimObjects always just get their own packet
	//This is the packet from createTypePacket
	virtual void loadFromPacket(ENetPacket const* const packet) = 0;

	//Create the packet on the server to be passed to loadFromPacket on the client
	virtual ENetPacket * createTypePacket() const = 0; 

	virtual ~NetType() {};
	NetType(SimObjectType _type) : type(_type) {}
};

