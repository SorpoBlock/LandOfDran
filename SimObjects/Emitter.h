#pragma once

#include "SimObject.h"
#include "Dynamic.h"
#include "ParticleTypes.h"

//What an Emitter's position comes from, sent with its state
enum EmitterAttachKind : unsigned char
{
	EmitterAttachFixed = 0,		//Its own position
	EmitterAttachDynamic = 1	//A dynamic, or the middle of one of its meshes
};

/*
	Ejects Blockland style particles, placed by server Lua with addEmitter
	Only its type and where it is are sent, each client ejects, moves, and draws the particles on its own, see Graphics/ParticleSystem.h
*/
class Emitter : public SimObject
{
	friend ObjHolder<Emitter>;

	//Update packets go out unreliably, so a change is sent this many times to make sure it arrives
	static constexpr int resendCount = 4;
	int updatesLeft = 0;

	//Index into ServerProgramData::emitterTypes
	uint16_t typeID = 0;
	EmitterAttachKind attachKind = EmitterAttachFixed;
	//Where it stays while fixed, where it was attached otherwise
	glm::vec3 position = glm::vec3(0);
	netIDType dynamicID = 0;
	//Which of the dynamic's meshes it follows the middle of, -1 for the dynamic itself
	int meshIndex = -1;

	void writeState(enet_uint8* dest) const;

	protected:

	explicit Emitter(uint16_t _typeID, const glm::vec3& _position);

	virtual void onCreation() override {}

	virtual void requestDestruction() override { dynamic.reset(); }

	public:

	//State written by both creation and update packets, after the net ID, creation packets put how old it is in between
	static constexpr unsigned int packetBytes = sizeof(uint16_t) + 2 + sizeof(netIDType) + sizeof(float) * 3;

	//Server: what it follows, it's removed along with it, see LoopServer::updateEmitters
	//Client: the dynamic with dynamicID, found again whenever that isn't it
	std::weak_ptr<Dynamic> dynamic;

	//Server only: the brick it was put on with attachToBrick, it's removed along with it
	netIDType brickID = NO_ID;

	//Client only: SDL_GetTicks when the server made it, for emitter types with a lifetime, can be before this client started
	int64_t startMS = 0;

	//Client only: see ParticleSystem::emit
	EmitterClock clock;

	uint16_t getTypeID() const { return typeID; }
	EmitterAttachKind getAttachKind() const { return attachKind; }
	netIDType getDynamicID() const { return dynamicID; }
	int getMeshIndex() const { return meshIndex; }

	//Server: its fixed position, or where the dynamic it follows is
	glm::vec3 getPosition() const;

	//Each of these has clients sent the change

	//Stays put there, no longer following or on anything
	void setPosition(const glm::vec3& _position);

	void setType(uint16_t _typeID);

	//meshIndex -1 follows the dynamic's position
	void attachToDynamic(std::shared_ptr<Dynamic> target, int _meshIndex);

	//Client: applies packetBytes of state written by the server
	void readFromPacket(const enet_uint8* src);

	virtual bool requiresNetUpdate() override;

	virtual unsigned int getCreationPacketBytes() const override;

	virtual unsigned int getUpdatePacketBytes() const override;

	virtual void addToCreationPacket(enet_uint8* dest) const override;

	virtual void addToUpdatePacket(enet_uint8* dest) override;
};
