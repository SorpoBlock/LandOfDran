#pragma once

#include "../LandOfDran.h"

#include "../NetTypes/DynamicType.h"

#include "../Physics/PhysicsWorld.h"

/*
	Struct holds state that may be needed to process packets from the client
*/
struct ServerProgramData
{
	std::shared_ptr<PhysicsWorld>	physicsWorld = nullptr;

	//Includes all of the types from each of the vectors below, used to send them all quickly when someone joins and needs types
	std::vector<std::shared_ptr<NetType>> allNetTypes;

	std::vector<std::shared_ptr<DynamicType>> dynamicTypes;
};
