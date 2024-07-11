#pragma once

#include "../SimObjects/Dynamic.h"

/*
	Bind to a dynamic, pass InputMap to it
	Server settings parameters of how this works through MovementSettingsPacket
*/
struct PlayerController
{
	//Some parameters here:

	//The thing this is controlling
	std::weak_ptr<Dynamic> target;

	//Call for each controller each frame, returns true if weak_ptr lock expired
	bool control(const std::shared_ptr<InputMap> input,float deltaT)
	{
		std::shared_ptr<Dynamic> targetLock = target.lock();
		if (!targetLock)
			return true;

		if (input->isCommandKeydown(WalkForward))
			targetLock->setVelocity(btVector3(0, 0, 10));
		if (input->isCommandKeydown(WalkBackward))
			targetLock->setVelocity(btVector3(0, 0, -10));
		if (input->isCommandKeydown(WalkLeft))
			targetLock->setVelocity(btVector3(10, 0, 0));
		if (input->isCommandKeydown(WalkRight))
			targetLock->setVelocity(btVector3(-10, 0, 0));

		return false;
	}
};
