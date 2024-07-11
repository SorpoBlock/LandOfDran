#include "MovementSettings.h"

bool MovementSettingsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < 1 + sizeof(netIDType))
		return true;

	netIDType id;
	memcpy(&id, packet->data + 1, sizeof(netIDType));

	std::shared_ptr<Dynamic> toUpdate = simulation.dynamics->find(id);
	if (!toUpdate)
		return false;

	//TODO: Pass controller parameters to the controller itself

	auto controller = std::make_shared<PlayerController>();
	controller->target = toUpdate;
	simulation.controllers.push_back(controller);

	return true;
}

MovementSettingsPacket::MovementSettingsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

MovementSettingsPacket::~MovementSettingsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
