#pragma once

#include "../NetTypes/DynamicType.h"
#include "../SimObjects/Dynamic.h"
#include "../SimObjects/StaticObject.h"
#include "../Networking/ObjHolder.h"
#include "../Graphics/PlayerCamera.h"
#include "../GameLoop/PlayerController.h"

/*
	Client only
	SimObjects and SimObjectTypes
	Any in-game object that has state managed by the server
	Created on server join, deleted when leaving server
*/
struct Simulation
{
	float serverLastSlowestFrame = 0.0;
	float serverAverageFrame = 0.0;

	//Only stored if we succesfully managed to log in to the server we're currently playing on
	std::string evalPassword = "";

	/*
		How many snapshots to hold onto, so we don't need to poll SettingsManager each updateSimObjects packet
	*/
	float idealBufferSize = 5.0;

	std::shared_ptr<Camera>			camera = nullptr;

	//Types:
	std::vector<std::shared_ptr<DynamicType>> dynamicTypes;

	//See TakeOverPhysicsPacket which can add or remove to this list
	std::vector<std::shared_ptr<Dynamic>> controlledDynamics;

	//Probably a lot of overlap between targets and controlledDynamics
	std::vector<std::shared_ptr<PlayerController>> controllers;

	//Objects (object holders):
	ObjHolder<Dynamic>* dynamics = nullptr;
	ObjHolder<StaticObject>* statics = nullptr;
};
