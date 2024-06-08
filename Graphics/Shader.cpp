#include "Shader.h"

/*
    A minmum 20Kb buffer reserved for loading text of shader files
    before they are passed to OpenGL.
    Will be increased if needed
*/
char * Shader::shaderTextStorage = 0;

/*
    The current size of shaderTextStorage in bytes
*/
int Shader::textStorageSize = 0;

Shader::Shader(std::string filePath, GLenum type)
{
    //Start our persistant text buffer at 20Kb
    if (!shaderTextStorage)
    {
        shaderTextStorage = new char[20000];
        textStorageSize = 20000;
    }

	scope("Shader::Shader");

	info("Creating shader from " + filePath);

	shaderType = type;
    switch (shaderType)
    {
        case GL_VERTEX_SHADER: break;
        case GL_TESS_CONTROL_SHADER: break;
        case GL_TESS_EVALUATION_SHADER: break;
        case GL_GEOMETRY_SHADER: break;
        case GL_FRAGMENT_SHADER: break;
        default:
            error("Invalid shader type requested: " + std::to_string(type));
            return;
    }

    handle = glCreateShader(type);
    if (!handle)
    {
        error("glCreateShader failed");
        error(std::to_string(glGetError()));
        return;
    }

    //It's a text file, but we're just copying it all as one big buffer for openGL to look at
    std::ifstream shaderFile(filePath.c_str(), std::ios::ate | std::ios::binary);
    if (!shaderFile.is_open())
    {
        error("Could not open " + filePath);
        return;
    }

    //Figure out how big file is then return to start position
    int size = (int)shaderFile.tellg();
    shaderFile.seekg(0);

    if (size < 1)
    {
        error("Error getting size of shader file.");
        return;
    }

    //Do we need a bigger text buffer
    if (size > textStorageSize)
    {
        delete[] shaderTextStorage;
        shaderTextStorage = new char[size];
        textStorageSize = size;
    }

    shaderFile.read(shaderTextStorage, size);

    glShaderSource(handle, 1, &shaderTextStorage, &size);
    glCompileShader(handle);

    shaderFile.close();

    //We need to get the expected length of the shader, then get the shader itself
    //length and actualLength should not differ
    int length, actualLength = 0;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);

    //Do we need a bigger text buffer
    if (length > textStorageSize)
    {
        delete[] shaderTextStorage;
        shaderTextStorage = new char[length];
        textStorageSize = length;
    }

    glGetShaderInfoLog(handle, length, &actualLength, shaderTextStorage);

    //If the length of the openGL error output is 0 then it succeeded
    if (!length)
        debug("Shader compiled successfully.");
    else
    {
        error("Error while compiling " + filePath + " see associated error file for more info!");

        std::string logPath = "Logs/ " + getFileFromPath(filePath) + ".log";
        
        std::ofstream shaderLog(logPath);
        if (shaderLog.is_open())
        {
            error(shaderTextStorage);
            shaderLog << shaderTextStorage;
            shaderLog.close();
        }
        else
            error("Could not open " + logPath + ".log for output");
    }
}

Shader::~Shader()
{
    glDeleteShader(handle);
}
