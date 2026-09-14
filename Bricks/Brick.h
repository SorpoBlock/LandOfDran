#pragma once

#include "../LandOfDran.h"
#include "BrickAttachments.h"

class btRigidBody;

//World units per stud horizontally and per plate vertically, same as the old game
constexpr float STUD_SIZE = 1.0f;
constexpr float PLATE_SIZE = 0.4f;

/*
	A brick on a grid of 1 stud x 1 plate x 1 stud, either a basic box or a special brick with its own shape filling the same space
	Plain data shared by server and client, owned by a BrickHolder
*/
struct Brick
{
	netIDType netId = 0;

	//Min corner, in studs horizontally and plates vertically
	int x = 0;
	int y = 0;
	int z = 0;

	//Before rotation, see footprintWidth and footprintLength
	//Special bricks always take their type's size
	unsigned char width = 1;
	unsigned char height = 1;
	unsigned char length = 1;

	//Quarter turns around the vertical axis, 0-3
	unsigned char angleID = 0;

	/*
		0 for a basic box, otherwise 1 more than the index of its SpecialBrickType in BrickTypes
		In network records it's the server's index instead, and clients map it to their own, see SpecialBrickTypesPacket
	*/
	uint16_t typeID = 0;

	glm::u8vec4 color = glm::u8vec4(255, 255, 255, 255);

	//Non-colliding bricks still have a body, so they can be clicked and raycast
	bool collides = true;

	//netId of the client that planted it, -1 for bricks from Lua or saves
	int ownerID = -1;

	std::string name = "";

	//Server only: its music loop, light, and emitter, nullptr for a brick with none, see BrickHolder::spawnAttachments
	std::shared_ptr<BrickAttachments> attachments = nullptr;

	btRigidBody* body = nullptr;

	//Position in BrickHolder's vector, for constant time removal
	unsigned int holderIndex = 0;

	//Client only: position in its InstancedBrickRenderer chunk, for constant time removal
	unsigned int renderIndex = 0;

	bool isSpecial() const { return typeID != 0; }

	//Radians counter-clockwise around +y seen from above, how a special brick's shape is turned in brick.vert and its physics body
	float getAngle() const { return angleID * 1.5707963f; }

	//Width and length swap places for quarter turns
	int footprintWidth() const { return (angleID % 2) ? length : width; }
	int footprintLength() const { return (angleID % 2) ? width : length; }

	glm::vec3 getWorldCenter() const
	{
		return glm::vec3((x + footprintWidth() * 0.5f) * STUD_SIZE, (y + height * 0.5f) * PLATE_SIZE, (z + footprintLength() * 0.5f) * STUD_SIZE);
	}
};
