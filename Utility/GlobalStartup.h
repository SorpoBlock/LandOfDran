#pragma once

#include "../LandOfDran.h"
#include "SettingManager.h"

/* 
	Calls functions like SDL_Init and such that don't return anything (important)
	Returns true if there was an issue and we should crash!
*/
bool globalStartup(SettingManager & settings);

//Calls shutdown for global start-up functions called in globalStartup
void globalShutdown();