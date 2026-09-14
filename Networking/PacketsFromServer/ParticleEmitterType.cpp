#include "ParticleEmitterType.h"

bool ParticleEmitterTypePacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	/*
		1 byte  - packet type
		1 byte  - ParticleEmitterTypeKind
		2 bytes - particle or emitter type ID
		The rest is from ParticleTypeData::write or EmitterTypeData::write
	*/

	//Like sound types these come while joining, before InGame
	if (packet->dataLength < 4)
		return true;

	uint16_t id;
	memcpy(&id, packet->data + 2, sizeof(uint16_t));
	size_t at = 4;

	if (packet->data[1] == ParticleTypeKind)
	{
		ParticleTypeData data;
		if (data.read(packet->data, packet->dataLength, at))
			pd.particles->setParticleType(id, data);
		else
			error("Particle type packet was too short");
	}
	else if (packet->data[1] == EmitterTypeKind)
	{
		EmitterTypeData data;
		if (data.read(packet->data, packet->dataLength, at))
			pd.particles->setEmitterType(id, data);
		else
			error("Emitter type packet was too short");
	}

	return true;
}

ParticleEmitterTypePacket::ParticleEmitterTypePacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

ParticleEmitterTypePacket::~ParticleEmitterTypePacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
