#pragma once

#include <limits>

#include "../SimObjects/Dynamic.h"
#include "../SimObjects/Emitter.h"
#include "../Graphics/PlayerCamera.h"
#include "../Networking/ClientPacketCreators.h"

/*
	Bind to a dynamic, pass InputMap to it
	Server settings parameters of how this works through MovementSettingsPacket
*/
struct PlayerController
{
	//Water level to pass to control when the world has no water
	static constexpr float noWater = -std::numeric_limits<float>::infinity();

	//Some parameters here:

	//The thing this is controlling
	std::weak_ptr<Dynamic> target;

	btQuaternion playerYaw = btQuaternion(0,0,0,1.0);

	bool serverSide = false;

	//Client only, set each frame by the client wrapper: our camera is in first person on the target, so its hidden body turns to face where we look
	//instead of where we walk, keeping what it holds (the flashlight) in front of it
	bool faceCamera = false;

	//Client uses these to send last inputs to server
	//Server cachces these and applies them each frame until a new packet comes in
	bool lastJump, lastJumpHeld, lastForward, lastBackward, lastLeft, lastRight;
	//Right mouse held, jetting if jetsAllowed
	bool lastJet = false;

	//Lua's client:setJetsEnabled on the server, the PlayerAbilities packet on the client
	bool jetsAllowed = true;

	//Server only: the jet flames under each foot while jetting, see LoopServer::updatePlayerAbilities
	std::weak_ptr<Emitter> jetEmitters[2];
	glm::vec3 lastCameraDirection = glm::vec3(0.01,1.0,0.01);
	//Server: taken from the client's periodic movement input packets. Client: taken from the local Camera each frame.
	//Used for cursor-based features (dynamic:snapToCursor, client:getCursorItem) - only meaningful once at least one
	//movement input packet has arrived, see the comment on the default value above
	glm::vec3 lastCameraPosition = glm::vec3(0,0,0);

	//Client only, last time we sent a packet to the server
	unsigned int lastSentControls = 0;
	//Client only, the jet state in the last packet we sent
	bool lastSentJet = false;

	//Whether the last control call made the target jump
	bool jumped = false;

	//getTicksMS of the last jump out of the water, swimming waits a moment after it so it doesn't slow the jump down
	unsigned int lastWaterJump = 0;

	//Client only, send last inputs to server for caching and reflection
	//Can return nullptr if object was deleted or packet was recently sent
	ENetPacket* makeMovementInputsPacket();

	//Server only wrapper
	bool controlWithLastInput(std::shared_ptr<PhysicsWorld> world, float deltaT, float waterLevel);

	/*
		Server and client side, called per frame, server caches last inputs from clients
		jump is a new press of the jump key, jumpHeld is whether it's down at all (it swims up)
		jet is whether right mouse is held, which lifts the player and speeds them along while jetsAllowed
		waterLevel is noWater if there's no water
	*/
	bool control(std::shared_ptr<PhysicsWorld> world, float deltaT, glm::vec3 cameraDirection, glm::vec3 cameraPosition, bool jump, bool jumpHeld, bool forward, bool backward, bool left, bool right, bool jet, float waterLevel);

	/*
	    Client side wrapper
		Call for each controller each frame, returns true if weak_ptr lock expired
	*/
	bool control(const std::shared_ptr<InputMap> input, const std::shared_ptr<Camera> camera, float deltaT, std::shared_ptr<PhysicsWorld> world, bool jet, float waterLevel);
};
