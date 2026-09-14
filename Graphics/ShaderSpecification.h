#pragma once

#include "../LandOfDran.h"
#include "Program.h"

/*
	Model matrix and material data for non-instanced rendering for a uniform buffer object
	Padding exists to conform to an std140 layout used by OpenGL
	OpenGL Size: 224 bytes
*/
struct BasicUniforms					
{														//base			aligned
	//Model matrix:
	glm::mat4 TranslationMatrix = glm::mat4(1.0);		//16*4			0
	glm::mat4 RotationMatrix = glm::mat4(1.0);			//16*4			64
	glm::mat4 ScaleMatrix = glm::mat4(1.0);				//16*4			128

	//Material uniforms:
	GLint useAlbedo = 0;								//4				192		after revisions, we got lucky here
	GLint useNormal = 0;								//4				196		there's nothing oddly sized that requires padding
	GLint useMetalness = 0;								//4				200
	GLint useRoughness = 0;								//4				204
	GLint useHeight = 0;								//4				208
	GLint useAO = 0;									//4				212
	
	GLint nonInstanced = 0;								//4				216
	GLint cameraSpacePosition = 0;						//4				220
};

/*
	Camera specific information for a uniform buffer object
	Size: 208 bytes
*/
struct CameraUniforms
{
	//Camera uniforms:
	glm::mat4 CameraProjection = glm::mat4(1.0);		//16*4			0
	glm::mat4 CameraView = glm::mat4(1.0);				//16*4			64
	glm::mat4 CameraAngle = glm::mat4(1.0);				//16*4			128
	glm::vec3 CameraPosition = glm::vec3(0.0);			//16			192   vec3 is technically only 12 bytes, insert 4 bytes padding
	float padding1 = 0;
	glm::vec3 CameraDirection = glm::vec3(0, 0, 1);		//16			208   padding doesn't matter here cause ints have alignment of 4 bytes
};

/*
	Time of day, lighting, fog, and water information for a uniform buffer object
	Every vec3 is followed by a float so std140 needs no extra padding
	Size: 112 bytes
*/
struct EnvironmentUniforms
{
	glm::vec3 SunDirection = glm::vec3(0, 1, 0);		//12			0
	float FogDistanceMin = 150;							//4				12
	glm::vec3 LightDirection = glm::vec3(0, 1, 0);		//12			16
	float FogDistanceMax = 290;							//4				28
	glm::vec3 LightColor = glm::vec3(1, 1, 1);			//12			32
	float WaveTime = 0;									//4				44
	glm::vec3 SkyColor = glm::vec3(0, 0, 1);			//12			48
	float WaterLevel = 0;								//4				60
	glm::vec3 FogColor = glm::vec3(1, 1, 1);			//12			64
	//Height of the highest flat surface, grass or water, sky.frag uses it to find the visible horizon
	float HorizonHeight = 0;							//4				76
	//World space plane for gl_ClipDistance[0], only matters while GL_CLIP_DISTANCE0 is enabled
	glm::vec4 ClipPlane = glm::vec4(0, 0, 0, 0);		//16			80
	glm::vec3 AmbientColor = glm::vec3(0, 0, 0);		//12			96
	//1 normally, fades to 0 as the sun or moon reaches the horizon
	float ShadowStrength = 1;							//4				108
};

/*
	Lights placed by server Lua for a uniform buffer object, filled by PointLights::update
	Size: 4624 bytes, every member is float aligned so the struct matches std140 as is
*/
struct PointLightUniforms
{
	//Most lights lighting the scene at once, and most of those with shadows, model.frag's array sizes have to match
	static constexpr int maxLit = 32;
	static constexpr int maxShadowed = 8;
	static constexpr int byteSize = 16 + 16 * 3 * maxLit + 64 * 6 * maxShadowed;

	GLint PointLightCount = 0;							//4				0
	float PointShadowTexelScale = 0;					//4				4
	float padding1 = 0;									//4				8
	float padding2 = 0;									//4				12
	glm::vec4 PointLightPositionRange[maxLit];			//16*32			16
	glm::vec4 PointLightColorShadow[maxLit];			//16*32			528
	glm::vec4 PointLightSpotDirection[maxLit];			//16*32			1040
	glm::mat4 PointShadowMatrices[6 * maxShadowed];		//64*48			1552
};

/*
	Handles a uniform buffer object and a few other uniform related things for programs
*/
class ShaderManager
{
	private:

	//A handle to the actual OpenGL uniform buffer object
	GLuint basicUBO, cameraUBO, environmentUBO, pointLightUBO;

	public:

	//See note on struct definition, this is passed to a uniform buffer object
	BasicUniforms basicUniforms;

	//See note on struct definition, this is passed to a uniform buffer object
	CameraUniforms cameraUniforms;

	//See note on struct definition, this is passed to a uniform buffer object
	EnvironmentUniforms environmentUniforms;

	//See note on struct definition, this is passed to a uniform buffer object
	PointLightUniforms pointLightUniforms;

	//Program for drawing the glow around point lights
	Program* coronaShader = new Program();

	//Program for drawing normal meshes to screen will full PBR based lighting
	Program* modelShader = new Program();

	//Program for drawing bricks to screen
	Program* brickShader = new Program();

	//Programs for drawing shadows of normal meshes and bricks into one shadow cascade at a time
	Program* modelShadowCascadeShader = new Program();
	Program* brickShadowCascadeShader = new Program();

	//Program for multiplying transparent bricks' colors into the shadow tint map
	Program* brickShadowTintShader = new Program();

	//Program for drawing the outline/highlight effect on top of normal models
	Program* outlineShader = new Program();

	//Program for drawing the sky behind everything else
	Program* skyShader = new Program();

	//Program for drawing the water surface
	Program* waterShader = new Program();

	//Program for tinting the whole screen while the camera is under the water
	Program* underwaterShader = new Program();

	/*
		Reads a text file to see where we should find the shader files for the above programs
		Returns true if there was an error with at least one shader compilation
	*/
	bool readShaderList(const std::string &filePath);
	
	//Push camera changes to GPU/OpenGL, see struct for further description
	void updateCameraUBO() const;

	//Sends changes to uniformBufferData to the GPU, see struct for further description
	void updateBasicUBO() const;

	//Sends changes to environmentUniforms to the GPU
	void updateEnvironmentUBO() const;

	//Sends changes to pointLightUniforms to the GPU
	void updatePointLightUBO() const;

	/*
		Associates contained UBOs with their definitions in the OpenGL shaders
		See function definition for what names they should have in shaders
	*/
	void bind(Program* target) const;

	ShaderManager();
	~ShaderManager();
};

