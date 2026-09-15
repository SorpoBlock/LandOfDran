#pragma once

#include "../LandOfDran.h"
#include "ShaderSpecification.h"
#include "Texture.h"
#include "../SimObjects/ParticleTypes.h"

#include <random>

/*
	Client only: Blockland style particles from whichever server we're on
	Emitters eject particles with emit, then update drops the dead ones and uploads the rest for render
	Where a particle is follows from where it started and how fast, so particles don't need to be stepped each frame
*/
class ParticleSystem
{
	struct Particle
	{
		glm::vec3 position = glm::vec3(0);
		glm::vec3 velocity = glm::vec3(0);
		//getNowMS when it was ejected
		double startMS = 0;
		float lifetimeMS = 1;
		//Its emitter's color, multiplied into its type's
		glm::vec4 tint = glm::vec4(1);
	};

	struct ParticleTypeSlot
	{
		bool defined = false;
		ParticleTypeData data;
		//nullptr draws plain squares of the particle's color
		Texture* texture = nullptr;
		std::vector<Particle> particles;

		//Which instances in the buffer are this type's, as of the last update
		GLsizei firstInstance = 0;
		GLsizei instanceCount = 0;
	};

	struct EmitterTypeSlot
	{
		bool defined = false;
		EmitterTypeData data;
	};

	std::vector<ParticleTypeSlot> particleTypes;
	std::vector<EmitterTypeSlot> emitterTypes;

	std::shared_ptr<TextureManager> textures = nullptr;

	//graphics/maxparticles, emitters don't eject more while this many are alive
	unsigned int maxParticles = 20000;
	unsigned int liveParticles = 0;

	//Per particle: position and size, color, then spin angle in radians
	static constexpr int instanceFloats = 9;
	std::vector<float> instances;
	GLuint vao = 0;
	GLuint buffer = 0;
	size_t bufferFloats = 0;

	//For the debug menu
	unsigned int drawnParticles = 0;

	std::mt19937 random;

	float randomRange(float low, float high);

	public:

	//Milliseconds as a double, so particle ages stay exact however long the game has been running
	static double getNowMS();

	void setMaxParticles(int count);

	//From ParticleEmitterTypePacket, loads the texture if the path is new
	void setParticleType(uint16_t id, const ParticleTypeData& data);
	void setEmitterType(uint16_t id, const EmitterTypeData& data);

	//nullptr if the server hasn't sent one with that ID
	const EmitterTypeData* getEmitterType(uint16_t id) const;

	//Names of every emitter type the server sent, in ID order, for the wrench dialog
	std::vector<std::string> getEmitterTypeNames() const
	{
		std::vector<std::string> names;
		for (const EmitterTypeSlot& slot : emitterTypes)
		{
			if (slot.defined && !slot.data.name.empty())
				names.push_back(slot.data.name);
		}
		return names;
	}

	/*
		Ejects every particle an emitter of this type owes since it last did, spread along the way it moved and turned since its last position and rotation
		Ejection directions are turned by rotation, velocity is that of what the emitter follows, for particle types with an inheritedVelFactor
		tint multiplies particle colors. With a target, particles spread around the way to it instead of the emitter's up, and only last until they get there
	*/
	void emit(EmitterClock& clock, uint16_t emitterTypeID, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& velocity, double nowMS,
		const glm::vec4& tint = glm::vec4(1), const glm::vec3* target = nullptr);

	//For an emitter that isn't ejecting right now: forgets the particles it owes, so it doesn't make up for them in one burst later
	static void skipEmission(EmitterClock& clock, const glm::vec3& position, const glm::quat& rotation, double nowMS);

	//Drops particles that lived out their lifetime and uploads the rest, except ones hidden past fogEnd
	void update(double nowMS, const glm::vec3& cameraPosition, float fogEnd);

	//Drawn after everything that writes depth, with the sun's cascades and point light shadow maps bound for lit particle types, sets its own blending
	void render(std::shared_ptr<ShaderManager> shaders, const glm::mat4* lightSpaceMatricies) const;

	//Forgets every particle and type, when leaving a server
	void clear();

	std::string getStats() const;

	explicit ParticleSystem(std::shared_ptr<TextureManager> _textures);
	~ParticleSystem();
};
