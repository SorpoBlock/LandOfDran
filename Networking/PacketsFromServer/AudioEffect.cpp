#include "AudioEffect.h"

bool AudioEffectPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte - packet type
		1 byte - preset name length, then the name, see ReverbPresets.h
	*/

	if (packet->dataLength < 2)
		return true;

	unsigned int length = packet->data[1];
	if (packet->dataLength < 2 + length)
		return true;

	pd.audio->setReverb(std::string((char*)packet->data + 2, length));

	return true;
}

AudioEffectPacket::AudioEffectPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

AudioEffectPacket::~AudioEffectPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
