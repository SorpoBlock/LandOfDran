#pragma once

#include "../LandOfDran.h"

/*
	Blockland style particle and emitter datablocks, registered by server Lua with addParticleType and addEmitterType
	Clients get every one as they join and again whenever Lua adds or redefines one, see ParticleEmitterTypePacket
	Units are Blockland's: angles in degrees, spin and phi velocity in degrees per second, times in milliseconds
*/

//Second byte of a ParticleEmitterType packet
enum ParticleEmitterTypeKind : unsigned char
{
	ParticleTypeKind = 0,
	EmitterTypeKind = 1
};

struct ParticleTypeData
{
	//Widest a particle can be, in studs
	static constexpr unsigned int maxSize = 256;

	std::string name = "";
	//Relative to the game folder, clients load it from their own copy
	std::string texturePath = "";

	//Keys over a particle's life: its color and size at times[a], a fraction of its lifetime, blended in between
	glm::vec4 colors[4] = { glm::vec4(1), glm::vec4(1), glm::vec4(1), glm::vec4(1) };
	float sizes[4] = { 1, 1, 1, 1 };
	float times[4] = { 0, 0.33f, 0.66f, 1 };

	//Fraction of its velocity a particle loses per second, per axis
	glm::vec3 drag = glm::vec3(0);
	//Acceleration in studs per second squared
	glm::vec3 gravity = glm::vec3(0);
	//How much of the velocity of what the emitter follows particles start with
	float inheritedVelFactor = 0;
	float lifetimeMS = 1000;
	//Each particle lives lifetimeMS plus or minus up to this much
	float lifetimeVarianceMS = 0;
	//Degrees per second
	float spinSpeed = 0;
	//Blended by its alpha if true, added on top of what's behind it if false
	bool useInvAlpha = false;
	//Drawn back to front, for alpha blended particles that overlap each other
	bool needsSorting = false;
	//Lit and shadowed by the sun, ambient light, and point lights like a surface of its color, for smoke and bubbles rather than fire and sparks
	bool lit = false;

	//Keeps everything in a range clients can draw, the time keys in order, and the variance under the lifetime
	void clampValues();

	//Everything after a packet's type, kind, and ID
	void write(std::vector<unsigned char>& bytes) const;

	//Returns false if the packet is too short
	bool read(const unsigned char* data, size_t length, size_t& at);

	//Color and size at a fraction of a particle's lifetime
	void getKeys(float lifeFraction, glm::vec4& color, float& size) const;
};

struct EmitterTypeData
{
	//Most particle types one emitter type can pick from
	static constexpr unsigned int maxParticleTypes = 16;

	std::string name = "";
	//For menus, not used yet
	std::string uiName = "";

	//IDs of the particle types it ejects, each particle is one of them picked at random
	std::vector<uint16_t> particleTypes;

	float ejectionPeriodMS = 100;
	//Each gap between particles is ejectionPeriodMS plus or minus up to this much
	float periodVarianceMS = 0;
	//Studs per second away from the emitter, plus or minus up to velocityVariance
	float ejectionVelocity = 2;
	float velocityVariance = 1;
	//How far from the emitter, along the way they're ejected, particles start
	float ejectionOffset = 0;
	//Degrees from straight up, each particle goes out at a random angle between these
	float thetaMin = 0;
	float thetaMax = 90;
	//Degrees per second the direction particles go out in turns around the vertical
	float phiReferenceVel = 0;
	//Degrees past that direction around the vertical a particle can go, 360 for every way
	float phiVariance = 360;
	//The server removes emitters of this type this long after they're made, 0 for never
	float lifetimeMS = 0;

	void clampValues();

	//Everything after a packet's type, kind, and ID
	void write(std::vector<unsigned char>& bytes) const;

	//Returns false if the packet is too short
	bool read(const unsigned char* data, size_t length, size_t& at);
};

//Client only: where one emitter is in ejecting its particles, see ParticleSystem::emit
struct EmitterClock
{
	bool started = false;
	//ParticleSystem::getNowMS times
	double startMS = 0;
	double nextEjectionMS = 0;
	double lastUpdateMS = 0;
	glm::vec3 lastPosition = glm::vec3(0);
	glm::quat lastRotation = glm::quat(1, 0, 0, 0);
};
