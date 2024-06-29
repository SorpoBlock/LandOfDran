#include "DisplayConsoleLine.h"

bool ConsoleLinePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	//Too short to have any message
	if (packet->dataLength < 3)
		return true;

	//Debug, error, or normal
	unsigned char type = packet->data[1];

	unsigned int length = packet->data[2];
	if (packet->dataLength < 3 + length)
		return true;

	std::string message((char*)packet->data + 3, length);
	pd.debugMenu->addLogLine({
		(type & 1) == 1,
		(type & 2) == 2,
		message
	});

	return true;
}

ConsoleLinePacket::ConsoleLinePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

ConsoleLinePacket::~ConsoleLinePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
