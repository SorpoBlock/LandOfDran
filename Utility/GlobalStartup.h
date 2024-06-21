#pragma once

#include "../LandOfDran.h"
#include "SettingManager.h"
#include "../External/Imgui/imgui.h"
#include "../External/Imgui/imgui_impl_sdl2.h"
#include "../External/Imgui/imgui_impl_opengl3.h"

/* 
	Calls functions like SDL_Init and such that don't return anything (important)
	Returns true if there was an issue and we should crash!
*/
bool globalStartup(std::shared_ptr<SettingManager> settings, const ExecutableArguments& cmdArgs);

//Calls shutdown for global start-up functions called in globalStartup
void globalShutdown(const ExecutableArguments& cmdArgs);