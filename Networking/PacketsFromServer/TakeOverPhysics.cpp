#include "TakeOverPhysics.h"

bool TakeOverPhysicsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 6)
		return false;

	unsigned char addOrRemove = packet->data[1];
	if (addOrRemove > 1)
	{
		error("Weird value for addOrRemove argument of TakeOverPhysics packet: "  + std::to_string(addOrRemove));
		return true;
	}

	netIDType netID = 0;
	memcpy(&netID, packet->data+2, sizeof(netIDType));

	std::shared_ptr<Dynamic> controlledObject = simulation.dynamics->find(netID);
	if (!controlledObject)
		return false; //Hopefully it just hasn't loaded in yet

	if (addOrRemove) //Add
	{
		controlledObject->clientControlled = true;
		controlledObject->body->setActivationState(DISABLE_DEACTIVATION);
		simulation.controlledDynamics.push_back(controlledObject);
	}
	else //Remove
	{
		controlledObject->body->setActivationState(ACTIVE_TAG); //Reenable deactivation
		controlledObject->clientControlled = false;
		simulation.controlledDynamics.erase(std::remove(simulation.controlledDynamics.begin(), simulation.controlledDynamics.end(), controlledObject), simulation.controlledDynamics.end());
	}

	return true;
}

TakeOverPhysicsPacket::TakeOverPhysicsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

TakeOverPhysicsPacket::~TakeOverPhysicsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
