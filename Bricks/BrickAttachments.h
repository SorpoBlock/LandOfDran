#pragma once

#include "../LandOfDran.h"

//Which parts a BrickAttachments has, the same bits as a brick record's flags byte in Land of Dran saves, where 1 is collision
#define BrickAttachment_Music 2
#define BrickAttachment_Light 4
#define BrickAttachment_Emitter 16

/*
	What the wrench dialog puts on a brick besides its collision and name: a music loop, a light, and an emitter
	Only the server keeps these, clients just see the sound loop, Light, and Emitter made from them, see LuaFunctions/BrickLua.h
*/
struct BrickAttachments
{
	//Longest music or emitter type name
	static constexpr size_t maxNameLength = 255;
	//Furthest a light can be from the middle of its brick along each axis, in world units
	static constexpr float maxLightOffset = 32.0f;

	//A sound type's name, "" for no music
	std::string musicName = "";
	float musicVolume = 1.0f;
	float musicPitch = 1.0f;

	bool hasLight = false;
	glm::vec3 lightColor = glm::vec3(1);
	float lightBrightness = 50.0f;
	float lightFlicker = 0.0f;
	float lightCoronaWidth = 0.0f;
	//0 shines every way, otherwise a spotlight's full width in degrees
	float lightConeAngle = 0.0f;
	glm::vec3 lightDirection = glm::vec3(0, -1, 0);
	float lightSpin = 0.0f;
	//From the middle of the brick, in world units. Point light shadows leave out bricks the light is inside, so it isn't buried by its own brick
	glm::vec3 lightOffset = glm::vec3(0);

	//An emitter type's name, "" for no emitter
	std::string emitterName = "";

	//Server only: the loop, light, and emitter made from the settings above, never saved or sent
	unsigned int musicLoopID = NO_ID;
	netIDType lightID = NO_ID;
	netIDType emitterID = NO_ID;

	//BrickAttachment bits for the parts it has
	unsigned char getFlags() const;
	bool isEmpty() const { return getFlags() == 0; }

	//Keeps every value in the range the loop, light, and emitter take them in, and names to maxNameLength
	void clampValues();

	//Every light setting back to what a newly added light starts with, in the middle of its brick, leaving hasLight alone
	void resetLight();

	/*
		Only the parts in getFlags, passed to writeBytes as they go
		Music: name length byte, name, volume and pitch floats
		Light: 15 floats, color, brightness, flicker, corona width, cone angle, direction, spin, offset
		Emitter: name length byte, name
	*/
	void writeParts(const std::function<void(const void*, size_t)>& writeBytes) const;

	//Reads the parts flags says follow, false if readBytes runs out
	bool readParts(unsigned char flags, const std::function<bool(void*, size_t)>& readBytes);

	//For packets: the flags byte then writeParts
	void write(std::vector<unsigned char>& bytes) const;
	bool read(const unsigned char* data, size_t length, size_t& at);
};
