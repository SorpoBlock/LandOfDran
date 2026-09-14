#include "VoiceStatus.h"

bool VoiceStatusPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte - packet type
		1 byte - VoiceStatusKind
		VoiceStatusMuted: 1 byte, whether the server muted us
		VoiceStatusTalkerName: 4 bytes net ID of a client, 1 byte name length, then the name
	*/

	if (!pd.voice || packet->dataLength < 2)
		return true;

	switch ((VoiceStatusKind)packet->data[1])
	{
		case VoiceStatusMuted:
		{
			if (packet->dataLength < 3)
				return true;

			pd.voice->setMuted(packet->data[2]);
			return true;
		}

		case VoiceStatusTalkerName:
		{
			unsigned int byteIterator = 2;
			if (packet->dataLength < byteIterator + sizeof(netIDType) + 1)
				return true;

			netIDType talker;
			memcpy(&talker, packet->data + byteIterator, sizeof(netIDType));
			byteIterator += sizeof(netIDType);

			unsigned int nameLength = packet->data[byteIterator];
			byteIterator += 1;

			if (packet->dataLength < byteIterator + nameLength)
				return true;

			pd.voice->setTalkerName(talker, std::string((char*)packet->data + byteIterator, nameLength));
			return true;
		}
	}

	return true;
}

VoiceStatusPacket::VoiceStatusPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

VoiceStatusPacket::~VoiceStatusPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
