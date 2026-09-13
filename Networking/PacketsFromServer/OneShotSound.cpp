#include "OneShotSound.h"

bool readSoundLocation(const ENetPacket* packet, unsigned int& byteIterator, Simulation& simulation, SoundLocation& result, bool& waiting)
{
	waiting = false;

	if (packet->dataLength < byteIterator + 1)
		return false;

	SoundLocationKind kind = (SoundLocationKind)packet->data[byteIterator];
	byteIterator++;

	switch (kind)
	{
		case SoundLocationFlat:
		{
			result = SoundLocation::flat();
			return true;
		}

		case SoundLocationFixed:
		{
			if (packet->dataLength < byteIterator + sizeof(float) * 3)
				return false;

			glm::vec3 position;
			memcpy(&position.x, packet->data + byteIterator, sizeof(float));
			memcpy(&position.y, packet->data + byteIterator + sizeof(float), sizeof(float));
			memcpy(&position.z, packet->data + byteIterator + sizeof(float) * 2, sizeof(float));
			byteIterator += sizeof(float) * 3;

			result = SoundLocation::at(position);
			return true;
		}

		case SoundLocationDynamic:
		{
			if (packet->dataLength < byteIterator + sizeof(netIDType))
				return false;

			netIDType id;
			memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
			byteIterator += sizeof(netIDType);

			//Lua can create a Dynamic and play a sound on it in the same tick, but the Dynamic is only sent afterwards
			std::shared_ptr<Dynamic> dynamic = simulation.dynamics ? simulation.dynamics->find(id) : nullptr;
			if (!dynamic)
			{
				waiting = true;
				return false;
			}

			result = SoundLocation::on(dynamic);
			return true;
		}
	}

	return false;
}

bool OneShotSoundPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte  - packet type
		2 bytes - sound ID
		4 bytes - pitch
		4 bytes - volume
		The rest - location, see readSoundLocation
	*/

	//A sound from before we loaded in would be late anyway
	if (cmdArgs.gameState != InGame)
		return true;

	unsigned int byteIterator = 1;
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

	pd.audio->playSound(soundID, where, pitch, volume);

	return true;
}

OneShotSoundPacket::OneShotSoundPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

OneShotSoundPacket::~OneShotSoundPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
