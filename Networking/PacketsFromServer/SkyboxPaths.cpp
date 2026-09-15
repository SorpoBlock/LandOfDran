#include "SkyboxPaths.h"

bool SkyboxPathsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte - packet type
		1 byte - day skybox path length, then the path, empty for the plain sky
		1 byte - night skybox path length, then the path
	*/

	if (packet->dataLength < 3)
		return true;

	unsigned int dayLength = packet->data[1];
	if (packet->dataLength < 3 + dayLength)
		return true;

	unsigned int nightLength = packet->data[2 + dayLength];
	if (packet->dataLength < 3 + dayLength + nightLength)
		return true;

	//Loaded by LoopClient::renderEverything, which has the image based lighting setting
	simulation.skyboxPaths[0] = std::string((char*)packet->data + 2, dayLength);
	simulation.skyboxPaths[1] = std::string((char*)packet->data + 3 + dayLength, nightLength);

	return true;
}

SkyboxPathsPacket::SkyboxPathsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

SkyboxPathsPacket::~SkyboxPathsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
