#pragma once

#include "../LandOfDran.h"
#include "../Utility/SettingManager.h"

#include "../External/Imgui/imgui.h"
#include "../External/Imgui/imgui_impl_sdl2.h"
#include "../External/Imgui/imgui_impl_opengl3.h"

struct RenderContext
{
	private:
		bool valid = false;
		unsigned int width = 0;
		unsigned int height = 0;
		bool mouseLocked = false;

		SDL_Window* window = 0;
		SDL_GLContext context;

	public:
		//Was the RenderContext initalized correctly
		bool isValid() const
		{
			return valid;
		}

		//Get resolution in pixels
		glm::vec2 getResolution() const
		{
			return glm::vec2(width, height);
		}

		bool getMouseLocked() const
		{
			return mouseLocked;
		}

		//Sets whether to trap the mouse in the center of the screen and hide it, i.e. first person controls
		void setMouseLock(bool locked);

		//Swaps buffers each frame
		void swap() const;

		//Sets the screen as the frame/render buffer to draw to
		void select() const;

		//Clears the screen
		void clear(float r, float g, float b, float a = 1.0, bool depth = true) const;

		//Changes screen resolution
		void setSize(unsigned int x, unsigned int y);

		//Asks SDL to resize the window, which comes back as a SDL_WINDOWEVENT_SIZE_CHANGED event
		void resizeWindow(unsigned int x, unsigned int y);

		//A windowed start uses state's remembered window/width and height if there are any, fullscreen always uses settings
		RenderContext(std::shared_ptr<SettingManager> settings, std::shared_ptr<SettingManager> state);
		~RenderContext();

		//Calls ImGui_ImplSDL2_InitForOpenGL
		bool bindImGui() const;
};
