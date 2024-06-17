#include "DefaultPreferences.h"

void populateDefaults(std::shared_ptr<SettingManager> settings)
{
	/*
		Be sure to have the overwrite parameter always be set to false for these
		Don't want to overwrite the user's intended settings
  		Leave last parameter blank if you don't want it to show up in the options GUI
    	Users can still edit it manually in the text file that's exported
	*/

	//Global SDL / GL settings:
	settings->addInt("graphics/openglmajor", 		3, false,		"OpenGL Verison Major",1,4);
	settings->addInt("graphics/openglminor", 		3, false,		"OpenGL Version Minor",0,10);
	settings->addInt("graphics/multisamplebuffers", 	1, false,		"Anti-Aliasing Buffers",0,10);
	settings->addInt("graphics/multisamplesamples", 	8, false,		"Anti-Aliasing Samples",0,64);
	settings->addBool("graphics/compatibilityprofile",false, false,	"Use OpenGL Compatability Profile");
	settings->addBool("graphics/debug", 		  false,false,			"OpenGL debug mode, useful if attaching RenderDoc");

	//Render context specific settings:
	settings->addInt("graphics/startresolutionx", 	1024, false,	"Program X resolution to start with",1,4096);
	settings->addInt("graphics/startresolutiony", 	1024, false,	"Program Y resolution to start with",1,4096);
	settings->addBool("graphics/startfullscreen", 	false, false,	"Start in fullscreen");
	settings->addBool("graphics/usevsync", 			false, false,	"Limit FPS to monitor refresh speed");

	//General settings
	settings->addBool("logger/verbose",				false, false,	"Enable verbose logging");
	settings->addFloat("input/mousesensitivity",		1.0f, false, 	"Mouse Sensitivitiy",0,10);
	settings->addBool("input/invertmousey",			false, false,	"Invert mouse Y axis");
	
	//Network settings
	settings->addInt("network/assumedbandwidth",		0, false, 		"ENet bandwidth assumption, 0 for dynamic",0,65535);
	settings->addString("network/guestname",			"Guest", false,	"Guest name if not logged in");
	settings->addString("network/lastip",			"localhost", false, "Last IP connected to");
	
	//GUI Settings
	settings->addFloat("gui/opacity",				0.75f, false,		"HUD Opacity",0,1);
	settings->addEnum("gui/scaling",					1, 				"GUI Scaling Factor", 	{"Small","Normal","Large","Largest"});

	//Audio settings
	settings->addFloat("audio/mastervolume",			0.5f, false,		"Master Volume",0,1);
	settings->addFloat("audio/musicvolume",			0.5f, false,		"Music Volume",0,1);

	//Graphics settings
	settings->addEnum("graphics/waterquality",		1, 	 			"Water Quality",	{"No Reflections", "Half Res Reflections", "Full Reflections"});
	settings->addEnum("graphics/shadowresolution",	1, 	 			"Shadow Resolution", 	{"2k Shadows","4k Shadows","8k Shadows"});
	settings->addEnum("graphics/shadowsoftness",		1,  			"Shadow Softness" , 	{"No PCF","2x PCF","4x PCF","8x PCF"});
	settings->addBool("graphics/shadowcolor",		true, false, 	"Use colored shadows");
	settings->addEnum("graphics/godrayquality",		1, 	 			"God ray samples", 	{"None","32 samples","64 samples","96 samples","128 samples"});
	settings->addEnum("graphics/spritedensity",		1, 				"Sprite density",	{"Low","Medium","High","Very High"});
}
