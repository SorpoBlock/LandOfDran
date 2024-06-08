#pragma once

#include "../LandOfDran.h"
#include "Program.h"

/*
	A struct with all the uniforms that will be passed to all shaders using a UniformBufferObject
	There will be other uniforms that only are used in certain shaders that are listen lower in this file
	Padding exists to conform to an std140 layout used by OpenGL
	OpenGL Size: 376 bytes
*/
struct GlobalUniforms					
{														//base			aligned
	//Model matrix:
	glm::mat4 TranslationMatrix = glm::mat4(1.0);		//16*4			0
	glm::mat4 RotationMatrix = glm::mat4(1.0);			//16*4			64
	glm::mat4 ScaleMatrix = glm::mat4(1.0);				//16*4			128

	//Camera uniforms:
	glm::mat4 CameraProjection = glm::mat4(1.0);		//16*4			192
	glm::mat4 CameraView = glm::mat4(1.0);				//16*4			256
	glm::vec3 CameraPosition = glm::vec3(0.0);			//16			320   vec3 is technically only 12 bytes, insert 4 bytes padding
	float padding1 = 0;
	glm::vec3 CameraDirection = glm::vec3(0, 0, 1);		//16			336   padding doesn't matter here cause ints have alignment of 4 bytes

	//Material uniforms:
	GLint useAlbedo = 0;								//4				352
	GLint useNormal = 0;								//4				356
	GLint useMetalness = 0;								//4				360
	GLint useRoughness = 0;								//4				364
	GLint useHeight = 0;								//4				368
	GLint useAO = 0;									//4				372
};

/*
	Handles a uniform buffer object and a few other uniform related things for programs
*/
class ShaderManager
{
	private:

	//A handle to the actual OpenGL uniform buffer object
	GLuint handle;

	public:

	/*
		The actual values we will pass to the GPU through a UBO
	*/
	GlobalUniforms globalUniforms;

	//Program for drawing normal meshes to screen will full PBR based lighting
	Program* modelShader = 0;


	/*
		Reads a text file to see where we should find the shader files for the above programs
		Returns true if there was an error with at least one shader compilation
	*/
	bool readShaderList(std::string filePath);
	
	//Sends changes to uniformBufferData to the GPU
	void updateUniformBlock();

	//Associates uniformBufferData with a struct by name glslName in a shader file used by target via a UBO
	void bind(Program* target,std::string glslName = "GlobalUniforms", int index = 1);

	ShaderManager();
	~ShaderManager();
};

