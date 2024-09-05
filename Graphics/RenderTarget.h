#pragma once

#include "Texture.h"

class RenderTarget
{
	GLuint frameBuffer = 0;
	GLuint renderBuffer = 0;
	GLenum* drawBuffers = nullptr;
	Texture* colorResult = nullptr;
	Texture* depthResult = nullptr;

	bool valid = false;

	public:

	bool isValid() const { return valid;  }

	struct RenderTargetSettings
	{
		int width = 800;
		int height = 800;
		int channels = 3;
		int layers = 1;
		bool useColor = true;
		bool useDepth = true;
		glm::vec4 clearColor = glm::vec4(0, 0, 0, 0);
		GLenum minFilter = GL_LINEAR;
		GLenum magFilter = GL_LINEAR;
	} settings;

	void bindDepthResult(TextureLocations loc) const { if (!depthResult) return; depthResult->bind(loc); }
	void bindColorResult(TextureLocations loc) const { if (!colorResult) return; colorResult->bind(loc); }

	RenderTarget(const RenderTargetSettings& _settings, std::shared_ptr<TextureManager> textures);
	~RenderTarget();

	void use();
};

