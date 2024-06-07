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

	//General settings
	settings.addBool("logger/verbose",		  false, false, "Enable verbose logging");
	settings.addFloat("input/mousesensitivity",       1.0, false, 	"Mouse Sensitivitiy");
	settings.addBool("input/invertmousey",		  false, false,	"Invert mouse Y axis");
	
	//Network settings
	settings.addInt("network/assumedbandwidth",	  0, false, 	"ENet bandwidth assumption, 0 for dynamic");
	settings.addString("network/guestname",		  "Guest", false,"Guest name if not logged in");
	settings.addString("network/lastip",		  "localhost", false, "Last IP connected to");
	
	//GUI Settings
	settings.addFloat("gui/opacity",		  0.8, false,   "HUD Opacity");
	settings.addInt("gui/scaling",			  1, false,	"GUI Scaling Factor");

	//Audio settings
	settings.addFloat("audio/mastervolume",		  0.5, false,   "Master Volume");
	settings.addFloat("audio/musicvolume",		  0.5, false,   "Music Volume");

	//Graphics settings
	settings.addInt("graphics/waterquality",	  1, false, 	"Water Quality");
	settings.addInt("graphics/shadowresolution",	  1, false, 	"Shadow Resolution");
	settings.addInt("graphics/shadowsoftness",	  1, false, 	"Shadow Softness");
	settings.addBool("graphics/shadowcolor",	  true, false, 	"Use colored shadows");
	settings.addInt("graphics/godrayquality",	  1, false, 	"God ray samples");
	settings.addInt("graphics/spritedensity",	  1, false,	"Sprite density");
}
