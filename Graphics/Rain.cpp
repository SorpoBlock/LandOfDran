#include "Rain.h"

//Seconds to go from no rain to a downpour or back
static constexpr float intensityFadeMS = 4000.0f;
//A downpour soaks everything in about this long, and it all dries in about this long once the rain stops
static constexpr float wettingMS = 20000.0f;
static constexpr float dryingMS = 90000.0f;
//How often the map is redrawn to catch up with vehicles while there are any
static constexpr float vehicleRedrawMS = 100.0f;

static constexpr int dropCounts[4] = { 0, 3000, 7000, 14000 };
static constexpr int splashCounts[4] = { 0, 250, 600, 1200 };
static constexpr int mapResolutions[4] = { 0, 1024, 1024, 2048 };

int Rain::maxDrops() const
{
	return dropCounts[std::clamp(quality, 0, 3)];
}

int Rain::maxSplashes() const
{
	return splashCounts[std::clamp(quality, 0, 3)];
}

void Rain::setQuality(int rainQuality, std::shared_ptr<TextureManager> textureManager)
{
	textures = textureManager;
	rainQuality = std::clamp(rainQuality, 0, 3);
	if (rainQuality == quality && (map || mapFailed || quality == 0))
		return;

	quality = rainQuality;
	mapFailed = false;

	//Made again at the new size the next time it's needed
	if (map && mapResolutions[quality] != mapResolution)
		map.reset();
	mapMoved = true;
}

void Rain::update(float deltaMS, float serverIntensity, const glm::vec3& cameraPosition)
{
	float target = std::clamp(serverIntensity, 0.0f, 1.0f);
	float step = deltaMS / intensityFadeMS;
	intensity = intensity < target ? std::min(target, intensity + step) : std::max(target, intensity - step);

	//Even a drizzle soaks everything eventually, it just takes longer
	if (intensity > 0.0f)
		wetness = std::min(1.0f, wetness + deltaMS / wettingMS * intensity);
	else
		wetness = std::max(0.0f, wetness - deltaMS / dryingMS);

	if (intensity <= 0.0f && wetness <= 0.0f)
		return;

	time = std::fmod(time + deltaMS / 1000.0, timeLoopSeconds);

	if (quality <= 0 || mapFailed)
		return;

	if (!map)
	{
		mapResolution = mapResolutions[quality];

		RenderTarget::RenderTargetSettings settings;
		settings.width = mapResolution;
		settings.height = mapResolution;
		settings.useColor = false;
		//Heights from either side of a wall's edge shouldn't blend into a slope
		settings.minFilter = GL_NEAREST;
		settings.magFilter = GL_NEAREST;
		map = std::make_shared<RenderTarget>(settings, textures);

		if (!map->isValid())
		{
			error("Couldn't make the rain map, there won't be any drops, splashes, or wet surfaces");
			map.reset();
			mapFailed = true;
			return;
		}
		mapMoved = true;
	}

	glm::vec3 center = glm::floor(cameraPosition / mapStep + 0.5f) * mapStep;
	if (center != mapCenter)
	{
		mapCenter = center;
		mapMoved = true;
	}
}

bool Rain::mapNeedsDrawing(unsigned int brickGeneration, bool anyVehicles, float deltaMS)
{
	if (!isActive())
		return false;

	sinceVehicleRedrawMS += deltaMS;
	bool vehiclesDue = anyVehicles && sinceVehicleRedrawMS >= vehicleRedrawMS;
	if (!mapMoved && brickGeneration == mapGeneration && !vehiclesDue)
		return false;

	mapMoved = false;
	mapGeneration = brickGeneration;
	sinceVehicleRedrawMS = 0.0f;
	return true;
}

glm::mat4 Rain::useMap() const
{
	map->use();

	//Looking straight down: world x and z go across the map, and depth is 0 at its top and 1 at its bottom
	//Winding comes out mirrored from a normal camera, the map is drawn without face culling so it doesn't matter
	float minX = mapCenter.x - mapSize * 0.5f;
	float minZ = mapCenter.z - mapSize * 0.5f;
	float top = mapCenter.y + mapReach;
	float bottom = mapCenter.y - mapReach;

	glm::mat4 matrix(0.0f);
	matrix[0][0] = 2.0f / mapSize;
	matrix[3][0] = -2.0f * minX / mapSize - 1.0f;
	matrix[2][1] = 2.0f / mapSize;
	matrix[3][1] = -2.0f * minZ / mapSize - 1.0f;
	matrix[1][2] = -2.0f / (top - bottom);
	matrix[3][2] = 2.0f * top / (top - bottom) - 1.0f;
	matrix[3][3] = 1.0f;
	return matrix;
}

void Rain::passUniforms(std::shared_ptr<ShaderManager> shaders) const
{
	EnvironmentUniforms& uniforms = shaders->environmentUniforms;
	bool active = isActive();
	uniforms.RainIntensity = active ? intensity : 0.0f;
	uniforms.RainWetness = active ? wetness : 0.0f;
	uniforms.RainMapTop = mapCenter.y + mapReach;
	uniforms.RainMapBottom = mapCenter.y - mapReach;
	uniforms.RainMapArea = glm::vec4(mapCenter.x - mapSize * 0.5f, mapCenter.z - mapSize * 0.5f, mapSize, mapSize / (float)std::max(mapResolution, 1));
}

void Rain::bindMap() const
{
	if (map)
		map->bindDepthResult(RainMap);
}

void Rain::render(std::shared_ptr<ShaderManager> shaders, GLuint emptyVao, float screenHeight) const
{
	if (!isActive() || intensity <= 0.0f)
		return;

	int drops = (int)(maxDrops() * intensity);
	int splashes = (int)(maxSplashes() * intensity);
	if (drops < 1 && splashes < 1)
		return;

	bindMap();

	//Seen from any side, see-through, and in front of or behind each other in any order
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(emptyVao);

	//World size of one pixel per unit away from the camera, so faraway drops stay at least a pixel wide
	float pixelScale = 2.0f / (shaders->cameraUniforms.CameraProjection[1][1] * std::max(screenHeight, 1.0f));

	if (drops > 0)
	{
		Program* program = shaders->rainShader;
		program->use();
		glUniform1f(program->getUniformLocation("rainTime"), (float)time);
		glUniform1f(program->getUniformLocation("pixelScale"), pixelScale);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, drops);
	}

	if (splashes > 0)
	{
		Program* program = shaders->rainSplashShader;
		program->use();
		glUniform1f(program->getUniformLocation("rainTime"), (float)time);
		//Two quads each, see rainSplash.vert
		glDrawArraysInstanced(GL_TRIANGLES, 0, 12, splashes);
	}

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
}

std::string Rain::getStats() const
{
	char text[96];
	snprintf(text, sizeof(text), "intensity %.2f, wetness %.2f%s", intensity, wetness, isActive() ? "" : " (inactive)");
	return text;
}

void Rain::clear()
{
	intensity = 0.0f;
	wetness = 0.0f;
	mapMoved = true;
}
