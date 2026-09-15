#include "VehicleBricksBroken.h"
#include "../../Physics/RadiusImpulse.h"

//A big explosion can break hundreds of bricks, which don't each need their own sound
static constexpr int maxBreakSounds = 6;

/*
	1 byte			-	packet type
	4 bytes			-	vehicle net ID
	2 bytes			-	how many bricks it had before they broke
	2 bytes			-	how many broke
	12 bytes		-	where the impulse that broke them was
	4 bytes			-	its strength
	2 bytes per brick	-	index of a broken brick in the vehicle's bricks from before any broke
*/
bool VehicleBricksBrokenPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	static constexpr unsigned int headerBytes = 1 + sizeof(netIDType) + sizeof(uint16_t) * 2 + sizeof(float) * 4;

	if (cmdArgs.gameState != InGame || !simulation.vehicles)
		return false;

	if (packet->dataLength < headerBytes)
		return true;

	netIDType id;
	uint16_t before, count;
	glm::vec3 center;
	float strength;
	size_t at = 1;
	memcpy(&id, packet->data + at, sizeof(netIDType));
	at += sizeof(netIDType);
	memcpy(&before, packet->data + at, sizeof(uint16_t));
	at += sizeof(uint16_t);
	memcpy(&count, packet->data + at, sizeof(uint16_t));
	at += sizeof(uint16_t);
	memcpy(&center[0], packet->data + at, sizeof(float) * 3);
	at += sizeof(float) * 3;
	memcpy(&strength, packet->data + at, sizeof(float));

	if (packet->dataLength < headerBytes + count * sizeof(uint16_t))
	{
		error("VehicleBricksBroken packet shorter than its brick count");
		return true;
	}

	//Its creation packet or bricks haven't all arrived yet
	std::shared_ptr<Vehicle> vehicle = simulation.vehicles->find(id);
	if (!vehicle || !vehicle->hasAllBricks())
		return false;

	//Someone who joined after they broke was only sent the bricks that were left
	if (vehicle->bricks.size() != before)
		return true;

	std::vector<uint16_t> indices(count);
	memcpy(indices.data(), packet->data + headerBytes, count * sizeof(uint16_t));

	//Copies, since the vehicle drops them before they become debris
	std::vector<Brick> broken;
	std::vector<glm::vec3> middles;
	glm::mat4 brickTransform = vehicle->getBrickTransform();
	for (uint16_t index : indices)
	{
		if (index >= vehicle->bricks.size())
			continue;

		broken.push_back(vehicle->bricks[index]);
		middles.push_back(glm::vec3(brickTransform * glm::vec4(vehicle->bricks[index].getWorldCenter(), 1.0f)));
	}

	//Out of its body first, so the debris doesn't start inside it
	vehicle->removeBricks(indices, &pd.brickTypes);

	//Like hammered bricks, plus the push the impulse gave everything else, on top of however fast the vehicle was going
	float radius = impulseRadius(strength);
	glm::quat turn = vehicle->renderedRotation;
	for (size_t a = 0; a < broken.size(); a++)
	{
		if (simulation.brickDebris)
		{
			float distance = glm::distance(middles[a], center);
			glm::vec3 velocity = vehicle->serverVelocity + impulseDirection(center, middles[a]) * strength * impulseFalloff(distance, radius);
			simulation.brickDebris->spawn(broken[a], btTransform(btQuaternion(turn.x, turn.y, turn.z, turn.w), g2b3(middles[a])), g2b3(velocity));
		}

		if ((int)a < maxBreakSounds)
			pd.audio->playSound("BrickBreak", SoundLocation::at(middles[a]));
	}

	return true;
}

VehicleBricksBrokenPacket::VehicleBricksBrokenPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

VehicleBricksBrokenPacket::~VehicleBricksBrokenPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
