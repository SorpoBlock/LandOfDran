#include "GlobalStartup.h"

//Calls functions like SDL_Init and such that don't return anything (important)
bool globalStartup(SettingManager& settings)
{
	info("Starting SDL");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
	{
		error("Could not start SDL, SDL_GetError: " + std::string(SDL_GetError()));
		return true;
	}

    debug("Setting SDL-GL attributes");

    if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) != 0)
        error("Setting attribute SDL_GL_DEPTH_SIZE failed: " + std::string(SDL_GetError()));

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, settings.getInt("graphics/openglmajor")) != 0)
        error("Setting attribute SDL_GL_CONTEXT_MAJOR_VERSION failed: " + std::string(SDL_GetError()));

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, settings.getInt("graphics/openglminor")) != 0)
        error("Setting attribute SDL_GL_CONTEXT_MINOR_VERSION failed: " + std::string(SDL_GetError()));

    if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, settings.getInt("graphics/mutlisamplebuffers")) != 0)
        error("Setting attribute SDL_GL_MULTISAMPLEBUFFERS failed: " + std::string(SDL_GetError()));

    if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, settings.getInt("graphics/multisamplesamples")) != 0)
        error("Setting attribute SDL_GL_MULTISAMPLESAMPLES failed: " + std::string(SDL_GetError()));
    
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        (settings["grahpics/compatibilityprofile"] == "true") ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY : SDL_GL_CONTEXT_PROFILE_CORE
    ) != 0)
    {
        error("Setting attribute SDL_GL_CONTEXT_PROFILE_MASK failed: " + std::string(SDL_GetError()));
    }

    if (settings["graphics/debug"] == "true")
    {
        if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) != 0)
            error("SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG) failed");
    }

    glEnable(GL_CULL_FACE);

	return false;
}

//Calls shutdown for global start-up functions called in globalStartup
void globalShutdown()
{
	info("Shutting down SDL");
	SDL_Quit();
}
