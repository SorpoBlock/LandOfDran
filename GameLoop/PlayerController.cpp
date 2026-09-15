#include "PlayerController.h"

#include "../Bricks/Brick.h"

//Tallest ledge a walking player steps onto without jumping
static constexpr float maxStepHeight = 4 * PLATE_SIZE;

//Surfaces with a normal steeper than this are walls to step over, flatter ones are ground to step onto
static constexpr float walkableNormalY = 0.7f;

//With more than this much under water, but not all of it, the player can jump out of the water
static constexpr float treadWaterDepth = 0.25f;

//Top swimming speed, water drag keeps the real speed somewhat under it
static constexpr float swimSpeed = 10.0f;

//MS, about how long swimming takes to get up to speed or turn, slower than walking so it feels like water
static constexpr float swimBlendTime = 150.0f;

//MS after jumping out of the water before swimming takes over again
static constexpr unsigned int waterJumpMS = 400;

//Jets cancel gravity and push up this much more, like the old game's upward gravity of 20
static constexpr float jetLift = 20.0f;
//Climbing faster than this, jets only hold the speed instead of adding to it
static constexpr float jetMaxRiseSpeed = 30.0f;
//Moving while jetting heads toward twice walking speed, taking longer to change direction, like the old game in the air
static constexpr float jetSpeedMultiplier = 2.0f;
static constexpr float jetBlendTime = 150.0f;

/*
	If the player is walking into a wall no taller than maxStepHeight with room above it, lifts them on top of it
	Works on anything solid except other dynamics, so walking into a loose object still pushes it
*/
static void stepUp(std::shared_ptr<PhysicsWorld> world, const std::shared_ptr<Dynamic>& player, const btVector3& walkDir)
{
	std::shared_ptr<Model> model = player->getType()->getModel();
	btVector3 halfExtents = g2b3(model->getColHalfExtents());

	btTransform bodyTransform = player->body->getWorldTransform();
	btTransform box = bodyTransform * btTransform(btQuaternion::getIdentity(), g2b3(model->getColOffset()));

	auto sweep = [&](const btVector3& from, const btVector3& to)
	{
		btTransform start = box;
		btTransform end = box;
		start.setOrigin(from);
		end.setOrigin(to);
		return world->boxSweep(halfExtents, start, end, player->body);
	};

	const btVector3 up = btVector3(0, 1, 0);
	const btVector3 center = box.getOrigin();

	//Slightly more than a frame of walking, so the step happens as the player reaches the ledge
	const btVector3 ahead = walkDir * 0.3f;

	//Only while standing on something, a jump or fall shouldn't grab ledges
	if (!sweep(center + up * 0.05f, center - up * 0.2f).body)
		return;

	SweepResult wall = sweep(center, center + ahead);
	if (!wall.body || wall.normal.getY() > walkableNormalY || wall.body->getUserIndex() == dynamicBody)
		return;

	//Probe from a little above the limit, a ledge exactly maxStepHeight tall would start the sweep already touching its top
	float probeHeight = maxStepHeight + 0.1f;

	//A low ceiling, like the top of a doorway, only lowers the probe, the step just can't rise past it
	//Stays well clear of it, sweeps count anything within the collision margin as touching
	SweepResult ceiling = sweep(center, center + up * probeHeight);
	if (ceiling.body)
		probeHeight = probeHeight * ceiling.fraction - 0.1f;

	//Room above the ledge
	btVector3 raised = center + up * probeHeight;
	if (probeHeight < 0.06f || sweep(raised, raised + ahead).body)
		return;

	SweepResult ledge = sweep(raised + ahead, center + ahead);
	if (!ledge.body || ledge.normal.getY() < walkableNormalY)
		return;

	float rise = probeHeight * (1.0f - ledge.fraction);
	if (rise < 0.05f || rise > maxStepHeight + 0.02f || rise + 0.01f > probeHeight)
		return;

	bodyTransform.setOrigin(bodyTransform.getOrigin() + up * (rise + 0.01f));
	player->body->setWorldTransform(bodyTransform);

	btVector3 velocity = player->getVelocity();
	velocity.setY(std::max(velocity.getY(), (btScalar)0));
	player->setVelocity(velocity);
}

/*
	Forward and backward follow the camera up and down as well, left and right stay level, jump held swims straight up
	While swimming the player holds their depth instead of sinking or floating up, letting go hands them back to buoyancy
*/
static void swim(const std::shared_ptr<Dynamic>& player, float deltaT, glm::vec3 cameraDirection, bool jumpHeld, bool forward, bool backward, bool left, bool right, btScalar submerged)
{
	const btVector3 up = btVector3(0, 1, 0);

	btVector3 look = g2b3(cameraDirection);
	if (look.length2() < 0.0001f)
		return;
	look.normalize();

	btVector3 side = look.cross(up);
	//Looking straight up or down
	if (side.length2() < 0.0001f)
		side = btVector3(1, 0, 0);
	else
		side.normalize();

	btVector3 swimDir = look * (float(forward) - float(backward)) + side * (float(right) - float(left));
	if (jumpHeld)
		swimDir += up;

	if (swimDir.length2() > 0.0001f)
		swimDir.normalize();
	else
		swimDir.setZero();

	btRigidBody* body = player->body;
	if (deltaT > 0 && body->getInvMass() > 0)
	{
		//Cancel out gravity and whatever the water holds up, which Dynamic::applyWaterForces adds on both client and server
		btScalar mass = 1.0f / body->getInvMass();
		body->applyCentralForce(-body->getGravity() * mass * (1 - player->buoyancy * submerged));
	}

	btScalar blend = 1.0f - std::exp(-deltaT / swimBlendTime);
	player->setVelocity(player->getVelocity().lerp(swimDir * swimSpeed, blend));
}

//Client only, send last inputs to server for caching and reflection
//Can return nullptr if object was deleted or packet was recently sent
//Always resends the current full state (not just on change) since this goes out unreliably -
//that way one dropped packet only leaves the server stale for one more interval instead of
//potentially forever if the player holds a key with no further state changes to trigger a resend
ENetPacket* PlayerController::makeMovementInputsPacket()
{
	//Jets starting or stopping go out right away, so the flames under the player don't lag behind
	if (getTicksMS() - lastSentControls < 100 && lastJet == lastSentJet)
		return nullptr;

	lastSentControls = getTicksMS();
	lastSentJet = lastJet;

	std::shared_ptr<Dynamic> targetLock = target.lock();
	if (!targetLock)
		return nullptr;

	return makeMovementInputs(
		targetLock->getID(),
		lastJump,
		lastJumpHeld,
		lastForward,
		lastBackward,
		lastLeft,
		lastRight,
		lastJet,
		lastCameraDirection,
		lastCameraPosition
	);
}

//Server only wrapper
bool PlayerController::controlWithLastInput(std::shared_ptr<PhysicsWorld> world, float deltaT, float waterLevel)
{
	serverSide = true;
	return control(world, deltaT, lastCameraDirection, lastCameraPosition, lastJump, lastJumpHeld, lastForward, lastBackward, lastLeft, lastRight, lastJet, waterLevel);
}

//Server and client side, called per frame, server caches last inputs from clients
bool PlayerController::control(std::shared_ptr<PhysicsWorld> world, float deltaT, glm::vec3 cameraDirection, glm::vec3 cameraPosition, bool jump, bool jumpHeld, bool forward, bool backward, bool left, bool right, bool jet, float waterLevel)
{
	lastCameraDirection = cameraDirection;
	lastCameraPosition = cameraPosition;
	lastJump = jump;
	lastJumpHeld = jumpHeld;
	lastForward = forward;
	lastBackward = backward;
	lastLeft = left;
	lastRight = right;
	lastJet = jet;
	jumped = false;

	//Prevent huge deltaTs from causing huge jumps (like when debugging and pausing the game for a while)
	deltaT = std::clamp(deltaT, 0.0f, 33.0f);

	std::shared_ptr<Dynamic> targetLock = target.lock();
	if (!targetLock)
		return true;

	targetLock->body->activate();

	//Turns the player's head, see Dynamic::lookDirection
	if (glm::length(cameraDirection) > 0.0001f && !glm::any(glm::isnan(cameraDirection)))
	{
		targetLock->lookDirection = glm::normalize(cameraDirection);
		targetLock->hasLook = true;
	}

	btScalar submerged = targetLock->getSubmergedFraction(waterLevel);

	if (jump)
	{
		//Make sure we are standing on the ground before we try and jump
		btTransform feetStart = targetLock->body->getWorldTransform();
		btTransform feetEnd = targetLock->body->getWorldTransform();
		feetStart.setOrigin(feetEnd.getOrigin() + btVector3(0.0,  1.0, 0.0));
		feetEnd.setOrigin(feetEnd.getOrigin()   + btVector3(0.0, -1.0, 0.0));
		btVector3 boxSize = g2b3(targetLock->getType()->getModel()->getColHalfExtents());

		btRigidBody *sweepResult = world->boxSweepTest(boxSize, feetStart, feetEnd, targetLock->body);

		//Or treading water with their head above it, so they can climb out onto something
		bool treadingWater = submerged > treadWaterDepth && submerged < 1;

		//TODO: Check if we're on the ground
		if (sweepResult || treadingWater)
		{
			targetLock->body->applyCentralImpulse(btVector3(0, 30, 0));
			jumped = true;

			if (!sweepResult)
				lastWaterJump = getTicksMS();
		}
	}

	//Past Dynamic::swimDepth they go wherever the camera points
	bool swimming = submerged >= Dynamic::swimDepth &&getTicksMS() - lastWaterJump >= waterJumpMS;

	//Not while swimming, which already decides how the player moves
	bool jetting = jet && jetsAllowed && !swimming;
	if (jetting && deltaT > 0 && targetLock->body->getInvMass() > 0)
	{
		btRigidBody* body = targetLock->body;
		btScalar mass = 1.0f / body->getInvMass();
		btScalar lift = targetLock->getVelocity().getY() < jetMaxRiseSpeed ? jetLift : 0.0f;
		body->applyCentralForce((btVector3(0, lift, 0) - body->getGravity()) * mass);
	}

	//TODO: Move this to a constructor or something
	targetLock->body->setAngularFactor(btVector3(0, 0, 0));

	float speed = 10.0;
	float blendTime = 50.0; //MS

	btVector3 dir = g2b3(cameraDirection);
	dir.setY(0);
	dir = dir.length2() > 0.00000001f ? dir.normalized() : btVector3(0, 0, 1);
	float cameraYaw = atan2(dir.getX(), dir.getZ());

	if (faceCamera && !serverSide)
	{
		playerYaw = playerYaw.slerp(btQuaternion(3.1415 + cameraYaw, 0, 0), std::min(deltaT / blendTime, 1.0f));

		//Don't want to compete with ControlledPhysics packets from the same client
		btTransform t = targetLock->body->getWorldTransform();
		t.setRotation(playerYaw);
		targetLock->body->setWorldTransform(t);
	}

	bool leftRightUsed = false;
	bool forwardBackUsed = false;
	btQuaternion leftRightTurn, forwardBackTurn;

	if (forward)
	{
		forwardBackUsed = true;
		forwardBackTurn = btQuaternion(3.1415 + cameraYaw, 0, 0);
	}
	else if (backward)
	{
		forwardBackUsed = true;
		forwardBackTurn = btQuaternion(0 + cameraYaw, 0, 0);
	}

	if (left)
	{
		leftRightUsed = true;
		leftRightTurn = btQuaternion(3.0 * (3.1415 / 2.0) + cameraYaw, 0.0, 0.0);
	}
	else if (right)
	{
		leftRightUsed = true;
		leftRightTurn = btQuaternion(3.1415 / 2.0 + cameraYaw, 0.0, 0.0);
	}

	bool moving = leftRightUsed || forwardBackUsed;

	if (!moving && !(swimming && jumpHeld))
	{
		targetLock->body->setFriction(1.0);
		targetLock->stop(0);
		targetLock->playWalkingAnimation = false;
		return false;
	}
	else
	{
		targetLock->body->setFriction(0.0);
		targetLock->play(0, true);
		targetLock->playWalkingAnimation = true;
	}

	if (swimming && !moving)
	{
		swim(targetLock, deltaT, cameraDirection, jumpHeld, forward, backward, left, right, submerged);
		return false;
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

	if (!faceCamera && !serverSide)
	{
		//TODO: This LERP isn't right
		playerYaw = playerYaw.slerp(turn, deltaT / blendTime);

		//Don't want to compete with ControlledPhysics packets from the same client
		btTransform t = targetLock->body->getWorldTransform();
		t.setRotation(playerYaw);
		targetLock->body->setWorldTransform(t);
	}

	if (swimming)
	{
		swim(targetLock, deltaT, cameraDirection, jumpHeld, forward, backward, left, right, submerged);
		return false;
	}

	btVector3 walkDir = btMatrix3x3(turn) * btVector3(0.0, 0.0, -1.0);

	stepUp(world, targetLock, walkDir);

	btVector3 oldVel = targetLock->getVelocity();
	float moveSpeed = jetting ? speed * jetSpeedMultiplier : speed;
	float moveBlendTime = jetting ? jetBlendTime : blendTime;
	//TODO: This LERP isn't right
	btVector3 newVel = oldVel.lerp(walkDir * moveSpeed, deltaT / moveBlendTime);
	newVel.setY(oldVel.getY());
	targetLock->setVelocity(newVel);

	return false;
}

/*
	Client side wrapper
	Call for each controller each frame, returns true if weak_ptr lock expired
*/
bool PlayerController::control(const std::shared_ptr<InputMap> input, const std::shared_ptr<Camera> camera, float deltaT, std::shared_ptr<PhysicsWorld> world, bool jet, float waterLevel)
{
	serverSide = false;
	faceCamera = camera->getFirstPerson() && camera->target.lock() == target.lock();
	return control(world, deltaT, camera->getDirection(), camera->getPosition(), input->pollCommand(Jump), input->isCommandKeydown(Jump), input->isCommandKeydown(WalkForward), input->isCommandKeydown(WalkBackward), input->isCommandKeydown(WalkLeft), input->isCommandKeydown(WalkRight), jet, waterLevel);
}
