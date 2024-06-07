#include "DefaultPreferences.h"

void populateDefaults(SettingManager& settings)
{
	/*
		Be sure to have the overwrite parameter always be set to false for these
		Don't want to overwrite the user's intended settings
  		Leave last parameter blank if you don't want it to show up in the options GUI
    		Users can still edit it manually in the text file that's exported
	*/

	//Global SDL / GL settings:
	settings.addInt("graphics/openglmajor", 	  3, false,     "");
	settings.addInt("graphics/openglminor", 	  3, false,     "");
	settings.addInt("graphics/multisamplebuffers", 	  1, false,     "");
	settings.addInt("graphics/multisamplesamples", 	  8, false,     "Anti-Aliasing samples");
	settings.addBool("graphics/compatibilityprofile", false, false, "");
	settings.addBool("graphics/debug", 		  false, false, "OpenGL debug mode, useful if attaching RenderDoc");

	//Render context specific settings:
	settings.addInt("graphics/startresolutionx", 	  1024, false,  "Program X resolution to start with");
	settings.addInt("graphics/startresolutiony", 	  1024, false,  "Program Y resolution to start with");
	settings.addBool("graphics/startfullscreen", 	  false, false, "Start in fullscreen");
	settings.addBool("graphics/usevsync", 		  false, false, "Limit FPS to monitor refresh speed");
}
