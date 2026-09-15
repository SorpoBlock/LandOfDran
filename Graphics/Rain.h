#pragma once

#include "../LandOfDran.h"
#include "ShaderSpecification.h"
#include "RenderTarget.h"

/*
	Client only: rain from the server's setRain, eased in and out
	Keeps a map of how high the topmost brick is at each spot around the camera, seen from straight above, so drops, splashes, and
	wet surfaces only show where nothing is overhead. See LoopClient::renderEverything for where the map is drawn
	While it isn't raining and everything has dried off, nothing is drawn, updated, or allocated
*/
class Rain
{
	//Eased toward what the server set
	float intensity = 0.0f;
	//0-1, how wet the surfaces rain reaches are, rises while it rains and dries off slowly after
	float wetness = 0.0f;

	//graphics/rainquality, 0 is off and only leaves the sound
	int quality = 2;
	std::shared_ptr<TextureManager> textures = nullptr;

	//Depth seen straight down onto the area around the camera, nullptr until it first rains
	std::shared_ptr<RenderTarget> map = nullptr;
	int mapResolution = 0;
	//Couldn't make the map, don't keep trying every frame
	bool mapFailed = false;
	//The map's middle, moved in whole mapStep steps so it only needs redrawing now and then
	glm::vec3 mapCenter = glm::vec3(0);
	//Since it was made or moved, it needs drawing before it's used
	bool mapMoved = true;
	//InstancedBrickRenderer::getGeneration as of the last time it was drawn
	unsigned int mapGeneration = 0;
	//Vehicles can be anywhere, so while there are any the map is redrawn this often
	float sinceVehicleRedrawMS = 0.0f;

	//Seconds for drops and splashes, loops every timeLoopSeconds
	double time = 0.0;

	//How many drops and splashes are drawn at full intensity, by quality
	int maxDrops() const;
	int maxSplashes() const;

	public:

	//World units across the map, how far its middle moves at a time, and how far above and below the camera it reaches
	//Bricks higher than that are flattened onto its top, so they still count as overhead
	static constexpr float mapSize = 256.0f;
	static constexpr float mapStep = 32.0f;
	static constexpr float mapReach = 400.0f;

	//Every drop falls the height of its box, and every splash plays, a whole number of times in this long. See rain.vert and rainSplash.vert
	static constexpr double timeLoopSeconds = 400.0;

	//graphics/rainquality: 0 (only the sound) to 3, off frees the map
	void setQuality(int rainQuality, std::shared_ptr<TextureManager> textureManager);

	//Every frame, before the environment uniforms are sent: eases toward serverIntensity, and moves the map along with the camera
	void update(float deltaMS, float serverIntensity, const glm::vec3& cameraPosition);

	//0-1, eased, the sound follows it too
	float getIntensity() const { return intensity; }
	float getWetness() const { return wetness; }

	//Raining or still wet with rain quality on, so the map is kept up and surfaces get wet
	bool isActive() const { return quality > 0 && map && (intensity > 0.0f || wetness > 0.0f); }

	//Whether the map is out of date and needs drawing this frame: it moved, the bricks changed, or it's time to catch up with vehicles
	bool mapNeedsDrawing(unsigned int brickGeneration, bool anyVehicles, float deltaMS);

	//Selects and clears the map, returns the matrix to draw bricks into it with. Leaves the map selected
	glm::mat4 useMap() const;

	//Sets the rain members of the environment uniforms, all 0 while inactive, doesn't upload them
	void passUniforms(std::shared_ptr<ShaderManager> shaders) const;

	void bindMap() const;

	//Drops and splashes around the camera, after everything else in the scene. emptyVao is any VAO, the shaders need no vertex data
	void render(std::shared_ptr<ShaderManager> shaders, GLuint emptyVao, float screenHeight) const;

	//For the debug menu
	std::string getStats() const;

	//Stops the rain and dries everything right away, for leaving a server
	void clear();
};
