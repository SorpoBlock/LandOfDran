#include "RenderTarget.h"

void RenderTarget::use()
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glViewport(0, 0, settings.width, settings.height); 
	glClearColor(settings.clearColor.r, settings.clearColor.g, settings.clearColor.b, settings.clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

RenderTarget::RenderTarget(const RenderTargetSettings& _settings,std::shared_ptr<TextureManager> textures)
{
	scope("RenderTarget::RenderTarget");
	debug("Setting up a render target");

	settings = _settings;

	if (!settings.useColor && !settings.useDepth)
	{
		error("Neither depth nor color selected for frame buffer!");
		return;
	}

	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	if (settings.useColor)
	{
		colorResult = textures->createBlankTexture(settings.width, settings.height, settings.channels, settings.layers);
		colorResult->setFilter(settings.magFilter, settings.minFilter);
		colorResult->addToFramebuffer();
		drawBuffers = new GLenum[1];
		drawBuffers[0] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);
	}
	else
		glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (settings.useDepth)
	{
		depthResult = textures->createBlankShadow32(settings.width, settings.height, settings.layers);
		depthResult->setFilter(settings.magFilter, settings.minFilter);
		depthResult->addToFramebuffer(GL_DEPTH_ATTACHMENT);
	}
	else
	{
		//Render buffers allow quicker write access to depth buffer if we don't need to read to it
		//We still want a depth buffer since we will do depth testing when rendering to this frame buffer
		glGenRenderbuffers(1, &renderBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, settings.width, settings.height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("Error creating frame buffer!");
	else
		valid = true;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RenderTarget::~RenderTarget()
{
	glDeleteFramebuffers(1, &frameBuffer);
	glDeleteRenderbuffers(1, &renderBuffer);

	if (colorResult)
		colorResult->markForCleanup();

	if (depthResult)
		depthResult->markForCleanup();

	if (drawBuffers)
		delete drawBuffers;
}