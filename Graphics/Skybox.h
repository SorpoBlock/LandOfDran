#pragma once

#include "../LandOfDran.h"
#include "ShaderSpecification.h"

//What fills one of Skybox's slots, also the values of DaySkybox and NightSkybox in SkyUniforms
enum SkyboxKind
{
	SkyboxProcedural = 0,	//Nothing loaded, sky.frag draws its gradient
	SkyboxImages = 1,		//Five or six screen color .png faces in the old game's layout, see Skybox::load
	SkyboxHDR = 2			//An equirectangular .hdr, which can also light the world, see graphics/imagebasedlighting
};

/*
	Client only: the day and night skies server Lua picked with setSkybox, as cube maps
	Either can be left empty for sky.frag's own gradient. With image based lighting on, a .hdr sky also lights
	everything in model.frag, water.frag, and particle.vert through an irradiance function and blurred reflections
*/
class Skybox
{
	public:

	//Cube map face size a .hdr is converted to, its reflections are blurred into the smaller mip levels
	static constexpr int hdrFaceSize = 512;
	//Levels below the full size one, each blurrier, the last is fully rough
	static constexpr int reflectionLevels = 5;
	//Brightest a .hdr pixel's channels get before it lights the world, so a sun baked into the image doesn't light
	//everything a second time, unshadowed, on top of the day cycle's sun. The sky itself is still drawn unclamped
	static constexpr float lightingClamp = 16.0f;

	//Paths as last sent by the server, day then night, "" for none
	std::string paths[2];

	//Loads or clears both slots if paths or the image based lighting setting changed since the last call
	void update(const std::string& dayPath, const std::string& nightPath, bool imageBasedLighting);

	//Binds both cube maps to SkyDay and SkyNight
	void bind() const;

	//Fills SkyUniforms for this frame, nightAmount is 0 while it's day and 1 at night, see Environment::skyboxBlend
	void passUniforms(std::shared_ptr<ShaderManager> shaders, float nightAmount) const;

	Skybox(std::shared_ptr<ShaderManager> shaders);
	~Skybox();

	private:

	struct Slot
	{
		SkyboxKind kind = SkyboxProcedural;
		GLuint cubeMap = 0;
		//Irradiance at unit normals as 9 spherical harmonic coefficients, already scaled for model.frag's skyIrradiance
		glm::vec3 irradiance[9];
		//Blurred reflections were made, only for .hdr skies with image based lighting on
		bool lit = false;
	};

	Slot slots[2];
	bool loadedWithLighting = false;
	bool anythingLoaded = false;

	//A 1x1 black cube map for empty slots, so the samplers always have a cube map bound
	GLuint emptyCubeMap = 0;

	std::shared_ptr<ShaderManager> shaders;

	void clearSlot(Slot& slot);
	void loadSlot(Slot& slot, const std::string& path, bool imageBasedLighting);

	//Fills levels 1 to reflectionLevels of target with source blurred by more and more roughness
	void prefilter(GLuint source, GLuint target);
};
