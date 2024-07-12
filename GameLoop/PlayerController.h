#pragma once

#include "../SimObjects/Dynamic.h"
#include "../Graphics/PlayerCamera.h"

/*
	Bind to a dynamic, pass InputMap to it
	Server settings parameters of how this works through MovementSettingsPacket
*/
struct PlayerController
{
	//Some parameters here:

	//The thing this is controlling
	std::weak_ptr<Dynamic> target;

	btQuaternion playerYaw = btQuaternion(0,0,0,1.0);

	//Call for each controller each frame, returns true if weak_ptr lock expired
	bool control(const std::shared_ptr<InputMap> input,const std::shared_ptr<Camera> camera,float deltaT)
	{
		//Prevent huge deltaTs from causing huge jumps (like when debugging and pausing the game for a while)
		deltaT = std::clamp(deltaT, 0.0f, 33.0f);

		std::shared_ptr<Dynamic> targetLock = target.lock();
		if (!targetLock)
			return true;

		if (input->pollCommand(Jump))
		{
			//TODO: Check if we're on the ground
			targetLock->setVelocity(targetLock->getVelocity() + btVector3(0, 30, 0));
		}


		//TODO: Move this to a constructor or something
		targetLock->body->setAngularFactor(btVector3(0, 0, 0));

		float speed = 10.0;
		float blendTime = 50.0; //MS

		btVector3 dir = g2b3(camera->getDirection());
		dir.setY(0);
		dir = dir.normalized();
		float cameraYaw = atan2(dir.getX(), dir.getZ());

		bool leftRightUsed = false;
		bool forwardBackUsed = false;
		btQuaternion leftRightTurn,forwardBackTurn;

		if (input->isCommandKeydown(WalkForward))
		{
			forwardBackUsed = true;
			forwardBackTurn = btQuaternion(3.1415 + cameraYaw, 0, 0);
		}
		else if(input->isCommandKeydown(WalkBackward))
		{
			forwardBackUsed = true;
			forwardBackTurn = btQuaternion(0 + cameraYaw, 0, 0);
		}

		if (input->isCommandKeydown(WalkLeft))
		{
			leftRightUsed = true;
			leftRightTurn = btQuaternion(3.0 * (3.1415 / 2.0) + cameraYaw, 0.0, 0.0);
		}
		else if (input->isCommandKeydown(WalkRight))
		{
			leftRightUsed = true;
			leftRightTurn = btQuaternion(3.1415 / 2.0 + cameraYaw, 0.0, 0.0);
		}

		if (!(leftRightUsed || forwardBackUsed))
		{
			targetLock->body->setFriction(1.0);
			targetLock->stop(0);
			return false;
		}
		else
		{
			targetLock->body->setFriction(0.0);
			targetLock->play(0, true);
		}

		btQuaternion turn;
		if (leftRightUsed)
		{
			if (forwardBackUsed)
				turn = leftRightTurn.slerp(forwardBackTurn, 0.5);
			else
				turn = leftRightTurn;
		}
		else if (forwardBackUsed)
			turn = forwardBackTurn;

		//TODO: This LERP isn't right
		playerYaw = playerYaw.slerp(turn, deltaT / blendTime);

		btVector3 walkDir = btMatrix3x3(turn) * btVector3(0.0, 0.0, -1.0);

		btTransform t = targetLock->body->getWorldTransform();
		t.setRotation(playerYaw);
		targetLock->body->setWorldTransform(t);

		btVector3 oldVel = targetLock->getVelocity();
		//TODO: This LERP isn't right
		btVector3 newVel = oldVel.lerp(walkDir * speed, deltaT / blendTime);
		newVel.setY(oldVel.getY());
		targetLock->setVelocity(newVel);

		return false;
	}
};
