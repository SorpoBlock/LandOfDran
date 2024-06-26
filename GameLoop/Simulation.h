#pragma once

#include "../NetTypes/DynamicType.h"
#include "../SimObjects/Dynamic.h"
#include "../Networking/ObjHolder.h"

/*
	SimObjects and SimObjectTypes
	Any in-game object that has state managed by the server
	Created on server join, deleted when leaving server
*/
struct Simulation
{
	//Types:
	std::vector<std::shared_ptr<DynamicType>> dynamicTypes;

	//Objects (object holders):
	ObjHolder<Dynamic>* dynamics = nullptr;
};
