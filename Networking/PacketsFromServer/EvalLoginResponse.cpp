#include "EvalLoginResponse.h"

bool EvalLoginResponsePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return true;

	if (packet->dataLength < 2)
		return true;

	if (packet->data[1] == 255)
	{
		pd.debugMenu->authenticate();
	}
	else if (packet->data[1] == 0)
	{
		pd.debugMenu->adminLoginComment = "Wrong password!";
	}
	else
		error("Strange response for EvalLoginResponse packet from server");

	return true;
}

EvalLoginResponsePacket::EvalLoginResponsePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

EvalLoginResponsePacket::~EvalLoginResponsePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
