#include "ChatMessageFromServer.h"

bool ChatMessagePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	//Too short to have any message
	if (packet->dataLength < 2)
		return true;

	unsigned int length = packet->data[1];
	if (packet->dataLength < 2 + length)
		return true;

	std::string message((char*)packet->data + 2, length);
	pd.chatWindow->addMessage(message);

	return true;
}

ChatMessagePacket::ChatMessagePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

ChatMessagePacket::~ChatMessagePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
