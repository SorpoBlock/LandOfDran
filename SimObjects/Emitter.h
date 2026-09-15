#pragma once

#include "SimObject.h"
#include "Dynamic.h"
#include "ParticleTypes.h"

class Vehicle;

//What an Emitter's position comes from, sent with its state
enum EmitterAttachKind : unsigned char
{
	EmitterAttachFixed = 0,		//Its own position
	EmitterAttachDynamic = 1,	//A dynamic, or the middle of one of its meshes
	EmitterAttachBrick = 2,		//Its own position, on a brick, which keeps it past its type's lifetime
	EmitterAttachVehicle = 3	//A spot on a vehicle, carried over from one of the bricks it was sliced from, which keeps it past its type's lifetime
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
	//Where it stays while fixed, how far from the middle of the dynamic (or its mesh) along the dynamic's (or mesh's) own axes for a dynamic, and where it is in the vehicle's body's space for a vehicle
	glm::vec3 position = glm::vec3(0);
	//The dynamic's net ID, or the vehicle's
	netIDType dynamicID = 0;
	//Which of the dynamic's meshes it follows the middle of, -1 for the dynamic itself
	int meshIndex = -1;
	//Multiplied into its particles' colors
	glm::u8vec4 color = glm::u8vec4(255, 255, 255, 255);
	//The dynamic whose aim its particles go toward, NO_ID for none, and how far that aim reaches, see aimWith
	netIDType aimDynamicID = NO_ID;
	float aimRange = 0;

	void writeState(enet_uint8* dest) const;

	protected:

	explicit Emitter(uint16_t _typeID, const glm::vec3& _position);

	virtual void onCreation() override {}

	virtual void requestDestruction() override { dynamic.reset(); vehicle.reset(); }

	public:

	//State written by both creation and update packets, after the net ID, creation packets put how old it is in between
	static constexpr unsigned int packetBytes = sizeof(uint16_t) + 2 + sizeof(netIDType) + sizeof(float) * 3 + 4 + sizeof(netIDType) + sizeof(float);

	//Server: what it follows, it's removed along with it, see LoopServer::updateEmitters
	//Client: the dynamic with dynamicID, found again whenever that isn't it
	std::weak_ptr<Dynamic> dynamic;

	//Same for a vehicle it's on
	std::weak_ptr<Vehicle> vehicle;

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
	netIDType getAimDynamicID() const { return aimDynamicID; }
	float getAimRange() const { return aimRange; }

	//Where it is on the vehicle it's attached to, in the vehicle body's space
	const glm::vec3& getVehicleOffset() const { return position; }

	//How far from the middle of the dynamic or mesh it follows it is, turned with it, see attachToDynamic
	const glm::vec3& getDynamicOffset() const { return position; }

	//Its color as 0-1 floats
	glm::vec4 getTint() const { return glm::vec4(color) / 255.0f; }

	//Client only: the dynamic with aimDynamicID, found again whenever that isn't it
	std::weak_ptr<Dynamic> aimer;

	//Server: its fixed position, or where the dynamic or vehicle it follows is
	glm::vec3 getPosition() const;

	//Each of these has clients sent the change

	//Stays put there, no longer following or on anything
	void setPosition(const glm::vec3& _position);

	void setType(uint16_t _typeID);

	//meshIndex -1 follows the dynamic's position, offset is in studs along the dynamic's (or mesh's) own axes, so it turns with it
	void attachToDynamic(std::shared_ptr<Dynamic> target, int _meshIndex, const glm::vec3& offset = glm::vec3(0));

	//Stays put at the brick's position, removed along with the brick, and never for its type's lifetime
	void attachToBrick(netIDType _brickID, const glm::vec3& brickPosition);

	//Goes along with a vehicle at offset in its body's space, removed along with it, and never for its type's lifetime
	void attachToVehicle(const std::shared_ptr<Vehicle>& target, const glm::vec3& offset);

	//Only sent if it's different
	void setColor(const glm::u8vec4& _color);

	/*
		Particles go toward what the aiming dynamic looks at, up to range studs from its eyes, and stop there
		Its type's thetaMin and thetaMax spread them around that direction instead of around up. nullptr aims it normally again
	*/
	void aimWith(std::shared_ptr<Dynamic> aiming, float range);

	//Client: applies packetBytes of state written by the server
	void readFromPacket(const enet_uint8* src);

	virtual bool requiresNetUpdate() override;

	virtual unsigned int getCreationPacketBytes() const override;

	virtual unsigned int getUpdatePacketBytes() const override;

	virtual void addToCreationPacket(enet_uint8* dest) const override;

	virtual void addToUpdatePacket(enet_uint8* dest) override;
};
