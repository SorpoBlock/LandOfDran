#include "SoundLoop.h"
#include "OneShotSound.h"

bool SoundLoopPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte  - packet type
		1 byte  - SoundLoopOperation
		4 bytes - loop ID
		Only for SoundLoopStart:
		2 bytes - sound ID
		4 bytes - pitch
		4 bytes - volume
		The rest - location, see readSoundLocation
	*/

	if (cmdArgs.gameState != InGame)
		return false;

	unsigned int byteIterator = 2;
	if (packet->dataLength < byteIterator + sizeof(unsigned int))
		return true;

	SoundLoopOperation operation = (SoundLoopOperation)packet->data[1];

	unsigned int loopID;
	memcpy(&loopID, packet->data + byteIterator, sizeof(unsigned int));
	byteIterator += sizeof(unsigned int);

	if (operation == SoundLoopStop)
	{
		pd.audio->stopLoop(loopID);
		return true;
	}

	if (operation != SoundLoopStart)
		return true;

	if (packet->dataLength < byteIterator + sizeof(uint16_t) + sizeof(float) * 2)
		return true;

	uint16_t soundID;
	memcpy(&soundID, packet->data + byteIterator, sizeof(uint16_t));
	byteIterator += sizeof(uint16_t);

	float pitch, volume;
	memcpy(&pitch, packet->data + byteIterator, sizeof(float));
	byteIterator += sizeof(float);
	memcpy(&volume, packet->data + byteIterator, sizeof(float));
	byteIterator += sizeof(float);

	SoundLocation where;
	bool waiting;
	if (!readSoundLocation(packet, byteIterator, simulation, where, waiting))
		return !waiting;

	pd.audio->startLoop(loopID, soundID, where, pitch, volume);

	return true;
}

SoundLoopPacket::SoundLoopPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

SoundLoopPacket::~SoundLoopPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
