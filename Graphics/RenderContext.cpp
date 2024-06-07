#include "RenderContext.h"

//Sets whether to trap the mouse in the center of the screen and hide it, i.e. first person controls
void RenderContext::setMouseLock(bool locked)
{
	mouseLocked = locked;
	SDL_SetRelativeMouseMode(locked ? SDL_TRUE : SDL_FALSE);
}

//Swaps buffers each frame
void RenderContext::swap()
{
	if(window)
		SDL_GL_SwapWindow(window);
}

//Sets the screen as the frame/render buffer to draw to
void RenderContext::select()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

//Clears the screen
void RenderContext::clear(float r, float g, float b, float a, bool depth)
{
	glClearColor(r, g, b, a);
	if (depth)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		glClear(GL_COLOR_BUFFER_BIT);
}

//Changes screen resolution
void RenderContext::setSize(unsigned int x, unsigned int y)
{
	//I really thought there would be more here
	//Viewport is handled in select()
	width = x;
	height = y;
}

RenderContext::RenderContext(SettingManager & settings) 
	: context((SDL_GLContext)nullptr) // Exists to suppress warning, SDL_GL_CreateContext can return 0 anyway
{
	
}

RenderContext::~RenderContext()
{

}