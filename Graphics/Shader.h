#pragma once

#include "../LandOfDran.h"

/*
	Contains a reference to an OpenGL shader 
	One or more of these are then bound to a Program object
*/
struct Shader
{
	/*
		A minmum 20Kb buffer reserved for loading text of shader files
		before they are passed to OpenGL.
		Will be increased if needed
	*/
	static char* shaderTextStorage;

	/*
		The current size of shaderTextStorage in bytes
	*/
	static int textStorageSize;

	GLenum shaderType = 0;
	GLuint handle = 0;
	Shader(std::string filePath, GLenum type);
	~Shader();
};
 