#include "VehicleDriver.h"

/*
	1 byte		-	packet type
	4 bytes		-	vehicle net ID
	4 bytes		-	net ID of the dynamic driving it, NO_ID for none

	LoopClient::placeVehicleDrivers seats the driver, or lets them out
*/
bool VehicleDriverPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame || !simulation.vehicles)
		return false;

	if (packet->dataLength < 1 + sizeof(netIDType) * 2)
		return true;

	netIDType id;
	memcpy(&id, packet->data + 1, sizeof(netIDType));

	//Hopefully it just hasn't arrived yet
	std::shared_ptr<Vehicle> vehicle = simulation.vehicles->find(id);
	if (!vehicle)
		return false;

	memcpy(&vehicle->driverID, packet->data + 1 + sizeof(netIDType), sizeof(netIDType));
	return true;
}

VehicleDriverPacket::VehicleDriverPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

VehicleDriverPacket::~VehicleDriverPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
