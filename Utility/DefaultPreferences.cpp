#include "DefaultPreferences.h"

void populateDefaults(SettingManager& settings)
{
	//Be sure to have the last parameter (overwrite) always be set to false for these
	//Don't want to overwrite the user's intended settings

	settings.addInt("graphics/openglmajor", 3, false);
	settings.addInt("graphics/openglminor", 3, false);
	settings.addInt("graphics/multisamplebuffers", 1, false);
	settings.addInt("graphics/multisamplesamples", 8, false);
	settings.addBool("graphics/compatibilityprofile", false, false);
	settings.addBool("graphics/debug", false, false);
}