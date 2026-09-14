#include "ParticleSystem.h"

#include <chrono>

//Most particles one emitter ejects in a frame, so a long hitch doesn't come out as one huge burst
static constexpr int maxEjectionsPerEmit = 512;

//An emitter that moved further than this since last frame jumped there, its particles aren't spread along the jump
static constexpr float maxSpreadDistance = 32.0f;

double ParticleSystem::getNowMS()
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

float ParticleSystem::randomRange(float low, float high)
{
	if (high <= low)
		return low;

	return std::uniform_real_distribution<float>(low, high)(random);
}

void ParticleSystem::setMaxParticles(int count)
{
	maxParticles = (unsigned int)std::max(count, 0);
}

void ParticleSystem::setParticleType(uint16_t id, const ParticleTypeData& data)
{
	scope("ParticleSystem::setParticleType");

	if (id >= particleTypes.size())
		particleTypes.resize(id + 1);

	ParticleTypeSlot& slot = particleTypes[id];

	if (!slot.defined || slot.data.texturePath != data.texturePath)
	{
		Texture* texture = nullptr;

		//File paths come from the server, don't let one reach outside the game's folder
		if (!isPathInsideGameFolder(data.texturePath))
			error("Not loading the texture for particle type " + data.name + ", its file isn't inside the game folder: " + data.texturePath);
		else
		{
			texture = textures->createTexture(data.texturePath);
			if (texture)
			{
				texture->setFilter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
				texture->setWrapping(GL_CLAMP_TO_EDGE);
			}
		}

		if (slot.texture)
			slot.texture->markForCleanup();
		slot.texture = texture;
	}

	slot.data = data;
	slot.data.clampValues();
	slot.defined = true;
}

void ParticleSystem::setEmitterType(uint16_t id, const EmitterTypeData& data)
{
	if (id >= emitterTypes.size())
		emitterTypes.resize(id + 1);

	emitterTypes[id].data = data;
	emitterTypes[id].data.clampValues();
	emitterTypes[id].defined = true;
}

const EmitterTypeData* ParticleSystem::getEmitterType(uint16_t id) const
{
	if (id >= emitterTypes.size() || !emitterTypes[id].defined)
		return nullptr;

	return &emitterTypes[id].data;
}

void ParticleSystem::skipEmission(EmitterClock& clock, const glm::vec3& position, const glm::quat& rotation, double nowMS)
{
	if (!clock.started)
	{
		clock.started = true;
		clock.startMS = nowMS;
	}

	clock.nextEjectionMS = std::max(clock.nextEjectionMS, nowMS);
	clock.lastUpdateMS = nowMS;
	clock.lastPosition = position;
	clock.lastRotation = rotation;
}

void ParticleSystem::emit(EmitterClock& clock, uint16_t emitterTypeID, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& velocity, double nowMS)
{
	const EmitterTypeData* type = getEmitterType(emitterTypeID);
	if (!type || type->particleTypes.empty())
	{
		skipEmission(clock, position, rotation, nowMS);
		return;
	}

	//The first particle comes out right away
	if (!clock.started)
	{
		clock.started = true;
		clock.startMS = nowMS;
		clock.nextEjectionMS = nowMS;
		clock.lastUpdateMS = nowMS;
		clock.lastPosition = position;
		clock.lastRotation = rotation;
	}

	double sinceLastMS = nowMS - clock.lastUpdateMS;
	bool spread = sinceLastMS > 0 && glm::distance(clock.lastPosition, position) < maxSpreadDistance;

	int ejections = 0;
	while (clock.nextEjectionMS <= nowMS && ejections < maxEjectionsPerEmit)
	{
		ejections++;
		double ejectedMS = clock.nextEjectionMS;
		clock.nextEjectionMS += std::max(1.0f, type->ejectionPeriodMS + randomRange(-type->periodVarianceMS, type->periodVarianceMS));

		if (liveParticles >= maxParticles)
			continue;

		//Particles owed from earlier in the frame start where and how the emitter was then, and are already that much older
		glm::vec3 from = position;
		glm::quat turned = rotation;
		if (spread)
		{
			float along = (float)std::clamp((ejectedMS - clock.lastUpdateMS) / sinceLastMS, 0.0, 1.0);
			from = glm::mix(clock.lastPosition, position, along);
			turned = glm::slerp(clock.lastRotation, rotation, along);
		}

		//Blockland's ejection direction: theta down from the emitter's up, phi around it, with phi turning over time
		double referencePhi = std::fmod(type->phiReferenceVel * (ejectedMS - clock.startMS) / 1000.0, 360.0);
		float phi = glm::radians((float)referencePhi + randomRange(0.0f, type->phiVariance));
		float theta = glm::radians(randomRange(type->thetaMin, type->thetaMax));
		glm::vec3 direction = turned * glm::vec3(std::cos(phi) * std::sin(theta), std::cos(theta), std::sin(phi) * std::sin(theta));

		size_t pick = type->particleTypes.size() > 1 ? std::uniform_int_distribution<size_t>(0, type->particleTypes.size() - 1)(random) : 0;
		uint16_t particleTypeID = type->particleTypes[pick];
		if (particleTypeID >= particleTypes.size() || !particleTypes[particleTypeID].defined)
			continue;

		ParticleTypeSlot& particleType = particleTypes[particleTypeID];
		const ParticleTypeData& data = particleType.data;

		Particle particle;
		particle.position = from + direction * type->ejectionOffset;
		particle.velocity = direction * (type->ejectionVelocity + randomRange(-type->velocityVariance, type->velocityVariance)) + velocity * data.inheritedVelFactor;
		particle.startMS = ejectedMS;
		particle.lifetimeMS = std::max(1.0f, data.lifetimeMS + randomRange(-data.lifetimeVarianceMS, data.lifetimeVarianceMS));
		particleType.particles.push_back(particle);
		liveParticles++;
	}

	//Hit the cap, the rest aren't made up for
	if (clock.nextEjectionMS <= nowMS)
		clock.nextEjectionMS = nowMS + type->ejectionPeriodMS;

	clock.lastUpdateMS = nowMS;
	clock.lastPosition = position;
	clock.lastRotation = rotation;
}

void ParticleSystem::update(double nowMS, const glm::vec3& cameraPosition, float fogEnd)
{
	struct Visible
	{
		float distanceSquared;
		glm::vec3 position;
		float size;
		glm::vec4 color;
		float angle;
	};
	static std::vector<Visible> visible;

	instances.clear();
	liveParticles = 0;
	drawnParticles = 0;

	for (ParticleTypeSlot& type : particleTypes)
	{
		type.firstInstance = (GLsizei)(instances.size() / instanceFloats);
		type.instanceCount = 0;

		std::erase_if(type.particles, [nowMS](const Particle& particle) { return nowMS - particle.startMS >= particle.lifetimeMS; });
		liveParticles += (unsigned int)type.particles.size();

		if (!type.defined || type.particles.empty())
			continue;

		const ParticleTypeData& data = type.data;
		visible.clear();

		for (const Particle& particle : type.particles)
		{
			float ageMS = std::max(0.0f, (float)(nowMS - particle.startMS));
			float seconds = ageMS / 1000.0f;

			glm::vec4 color;
			float size;
			data.getKeys(ageMS / particle.lifetimeMS, color, size);
			if (size <= 0.0f || color.a <= 0.0f)
				continue;

			glm::vec3 position = particle.position;
			for (int axis = 0; axis < 3; axis++)
			{
				float drag = data.drag[axis];
				float gravity = data.gravity[axis];
				float velocity = particle.velocity[axis];

				if (drag < 0.0001f)
					position[axis] += velocity * seconds + 0.5f * gravity * seconds * seconds;
				else
				{
					//Solves dv/dt = gravity - drag * v, velocity eases toward gravity / drag
					float terminal = gravity / drag;
					position[axis] += terminal * seconds + (velocity - terminal) * (1.0f - std::exp(-drag * seconds)) / drag;
				}
			}

			glm::vec3 toCamera = position - cameraPosition;
			float distanceSquared = glm::dot(toCamera, toCamera);
			float reach = fogEnd + size;
			if (distanceSquared > reach * reach)
				continue;

			visible.push_back({ distanceSquared, position, size, color, glm::radians(data.spinSpeed * seconds) });
		}

		if (data.needsSorting)
			std::sort(visible.begin(), visible.end(), [](const Visible& a, const Visible& b) { return a.distanceSquared > b.distanceSquared; });

		for (const Visible& particle : visible)
		{
			instances.insert(instances.end(), { particle.position.x, particle.position.y, particle.position.z, particle.size,
				particle.color.r, particle.color.g, particle.color.b, particle.color.a, particle.angle });
		}

		type.instanceCount = (GLsizei)visible.size();
		drawnParticles += (unsigned int)visible.size();
	}

	if (instances.empty())
		return;

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	if (instances.size() > bufferFloats)
	{
		bufferFloats = instances.size() * 2;
		glBufferData(GL_ARRAY_BUFFER, bufferFloats * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(float), instances.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::render(std::shared_ptr<ShaderManager> shaders, const glm::mat4* lightSpaceMatricies) const
{
	if (drawnParticles < 1)
		return;

	Program* program = shaders->particleShader;
	program->use();
	GLint additiveUniform = program->getUniformLocation("additive");
	GLint useTextureUniform = program->getUniformLocation("useTexture");
	GLint litUniform = program->getUniformLocation("lit");
	glUniformMatrix4fv(program->getUniformLocation("lightSpaceMatricies"), 3, GL_FALSE, &lightSpaceMatricies[0][0][0]);

	//Billboards face the camera, but the mirrored water reflection camera flips their winding
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	const GLsizei stride = instanceFloats * sizeof(float);

	//Additive particles look the same in any order, so they go first and alpha blended ones cover them
	for (int pass = 0; pass < 2; pass++)
	{
		bool alphaBlended = pass == 1;

		for (const ParticleTypeSlot& type : particleTypes)
		{
			if (type.instanceCount < 1 || type.data.useInvAlpha != alphaBlended)
				continue;

			glBlendFunc(GL_SRC_ALPHA, alphaBlended ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE);
			glUniform1i(additiveUniform, !alphaBlended);
			glUniform1i(useTextureUniform, type.texture != nullptr);
			glUniform1i(litUniform, type.data.lit);
			if (type.texture)
				type.texture->bind(ParticleTexture);

			//OpenGL 3.3 has no base instance for glDrawArraysInstanced, so the attributes start at this type's instances instead
			size_t offset = (size_t)type.firstInstance * stride;
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)offset);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(offset + 4 * sizeof(float)));
			glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(offset + 8 * sizeof(float)));
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, type.instanceCount);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
}

void ParticleSystem::clear()
{
	for (ParticleTypeSlot& type : particleTypes)
	{
		if (type.texture)
			type.texture->markForCleanup();
	}

	particleTypes.clear();
	emitterTypes.clear();
	instances.clear();
	liveParticles = 0;
	drawnParticles = 0;
}

std::string ParticleSystem::getStats() const
{
	return std::to_string(liveParticles) + " alive, " + std::to_string(drawnParticles) + " drawn";
}

ParticleSystem::ParticleSystem(std::shared_ptr<TextureManager> _textures) : textures(_textures), random(std::random_device{}())
{
	//particle.vert makes each quad's corners from gl_VertexID, so the only attributes are per particle
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &buffer);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	for (GLuint attribute = 0; attribute < 3; attribute++)
	{
		glEnableVertexAttribArray(attribute);
		glVertexAttribDivisor(attribute, 1);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ParticleSystem::~ParticleSystem()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &buffer);
}
