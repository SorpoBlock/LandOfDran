#include "Emitter.h"

Emitter::Emitter(uint16_t _typeID, const glm::vec3& _position) : typeID(_typeID), position(_position)
{
}

glm::vec3 Emitter::getPosition() const
{
	if (attachKind == EmitterAttachDynamic)
	{
		std::shared_ptr<Dynamic> target = dynamic.lock();
		if (target)
			return b2g3(target->getPosition());
	}

	return position;
}

void Emitter::setPosition(const glm::vec3& _position)
{
	position = _position;
	attachKind = EmitterAttachFixed;
	dynamic.reset();
	dynamicID = 0;
	meshIndex = -1;
	brickID = NO_ID;
	updatesLeft = resendCount;
}

void Emitter::setType(uint16_t _typeID)
{
	typeID = _typeID;
	updatesLeft = resendCount;
}

void Emitter::attachToDynamic(std::shared_ptr<Dynamic> target, int _meshIndex)
{
	position = b2g3(target->getPosition());
	attachKind = EmitterAttachDynamic;
	dynamic = target;
	dynamicID = target->getID();
	meshIndex = _meshIndex;
	brickID = NO_ID;
	updatesLeft = resendCount;
}

void Emitter::writeState(enet_uint8* dest) const
{
	memcpy(dest, &typeID, sizeof(uint16_t));
	dest[2] = attachKind;
	dest[3] = meshIndex < 0 ? 255 : (enet_uint8)meshIndex;
	memcpy(dest + 4, &dynamicID, sizeof(netIDType));
	memcpy(dest + 4 + sizeof(netIDType), &position[0], sizeof(float) * 3);
}

void Emitter::readFromPacket(const enet_uint8* src)
{
	uint16_t newType;
	memcpy(&newType, src, sizeof(uint16_t));

	//A different type starts ejecting on its own schedule
	if (newType != typeID)
		clock = EmitterClock();
	typeID = newType;

	attachKind = src[2] == EmitterAttachDynamic ? EmitterAttachDynamic : EmitterAttachFixed;
	meshIndex = src[3] == 255 ? -1 : src[3];
	memcpy(&dynamicID, src + 4, sizeof(netIDType));
	memcpy(&position[0], src + 4 + sizeof(netIDType), sizeof(float) * 3);
}

bool Emitter::requiresNetUpdate()
{
	flaggedForUpdate = updatesLeft > 0;
	return flaggedForUpdate;
}

unsigned int Emitter::getCreationPacketBytes() const
{
	return sizeof(netIDType) + sizeof(uint32_t) + packetBytes;
}

unsigned int Emitter::getUpdatePacketBytes() const
{
	return packetBytes;
}

void Emitter::addToCreationPacket(enet_uint8* dest) const
{
	netIDType id = getID();
	memcpy(dest, &id, sizeof(netIDType));

	//So someone joining partway through an emitter's lifetime doesn't see it start over
	uint32_t ageMS = SDL_GetTicks() - creationTime;
	memcpy(dest + sizeof(netIDType), &ageMS, sizeof(uint32_t));

	writeState(dest + sizeof(netIDType) + sizeof(uint32_t));
}

void Emitter::addToUpdatePacket(enet_uint8* dest)
{
	writeState(dest);
	if (updatesLeft > 0)
		updatesLeft--;
}
