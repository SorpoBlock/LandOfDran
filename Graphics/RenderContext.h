#pragma once

#include "../LandOfDran.h"
#include "../Utility/SettingManager.h"

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
		void swap();

		//Sets the screen as the frame/render buffer to draw to
		void select();

		//Clears the screen
		void clear(float r, float g, float b, float a = 1.0, bool depth = true);

		//Changes screen resolution
		void setSize(unsigned int x, unsigned int y);

		RenderContext(SettingManager& settings);
		~RenderContext();
};
