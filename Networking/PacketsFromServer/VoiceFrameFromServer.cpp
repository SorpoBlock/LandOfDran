#include "VoiceFrameFromServer.h"
#include "OneShotSound.h"

bool VoiceFrameFromServerPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte   - packet type
		4 bytes  - net ID of the client talking
		1 byte   - flags, see VoiceFlag_End
		2 bytes  - sequence number
		Where the talker is, see readSoundLocation
		The rest - one Opus frame, nothing with VoiceFlag_End
	*/

	if (cmdArgs.gameState != InGame || !pd.voice)
		return true;

	unsigned int byteIterator = 1;
	if (packet->dataLength < byteIterator + sizeof(netIDType) + 1 + sizeof(uint16_t))
		return true;

	netIDType talker;
	memcpy(&talker, packet->data + byteIterator, sizeof(netIDType));
	byteIterator += sizeof(netIDType);

	unsigned char flags = packet->data[byteIterator];
	byteIterator += 1;

	uint16_t sequence;
	memcpy(&sequence, packet->data + byteIterator, sizeof(uint16_t));
	byteIterator += sizeof(uint16_t);

	//Unlike sounds, voice can't wait for the talker's player to arrive, it would be too late to play by then
	SoundLocation where;
	bool waiting;
	if (!readSoundLocation(packet, byteIterator, simulation, where, waiting))
		return true;

	pd.voice->receive(talker, flags, sequence, where, packet->data + byteIterator, (unsigned int)(packet->dataLength - byteIterator));

	return true;
}

VoiceFrameFromServerPacket::VoiceFrameFromServerPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

VoiceFrameFromServerPacket::~VoiceFrameFromServerPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
