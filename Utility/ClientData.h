#pragma once

#include "../LandOfDran.h"
#include "../Graphics/RenderContext.h"
#include "SettingManager.h"

/*
	A struct containing anything needed to actually play the game, as opposed to host it
	There should only ever be one of these
	This should not have any member functions, it's just to pass around what would otherwise be global state
*/
struct ClientData
{
	RenderContext* renderContext = 0;
	SettingManager *preferences = 0;
};
