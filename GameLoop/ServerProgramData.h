#pragma once

#include "../LandOfDran.h"

#include "../Networking/ObjHolder.h"
#include "../NetTypes/DynamicType.h"
#include "../SimObjects/Dynamic.h"

#include "../Physics/PhysicsWorld.h"

netIDType ObjHolder<Dynamic>::lastNetID = 0;

/*
	Struct holds state that may be needed to process packets from the client
*/
struct ServerProgramData
{
	std::shared_ptr<PhysicsWorld>	physicsWorld = nullptr;

	//Includes all of the types from each of the vectors below, used to send them all quickly when someone joins and needs types
	std::vector<std::shared_ptr<NetType>> allNetTypes;
	//Various specific kinds of types
	std::vector<std::shared_ptr<DynamicType>> dynamicTypes;

	//ObjHolders created and destroyed with ServerLoop class
	//All dynamic objects:
	ObjHolder<Dynamic>* dynamics = nullptr;
};
