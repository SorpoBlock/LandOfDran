#pragma once

#include "../LandOfDran.h"
#include "SettingManager.h"

/*
	In this function put settings.add* lines to set what the default settings should be
	These will be written to the text file if it's called in-between an import and export
*/
void populateDefaults(SettingManager& settings);
