#pragma once

#include "../LandOfDran.h"
#include "SettingManager.h"

enum SettingGuiScaling
{
	Small = 0,
	Normal = 1,
	Large = 2,
	Largest = 3
};

enum SettingWaterQuality
{
	NoReflections = 0,
	HalfReflections = 1,
	FullReflections = 2
};

enum SettingShadowResolution
{
	TwoK = 0,
	FourK = 1,
	EightK = 2
};

enum SettingShadowSoftness
{
	SoftnessNone = 0,
	Softness2x = 1,
	Softness4x = 2,
	Softness8x = 3
};

enum SettingGodRay
{
	RayNone = 0,
	Ray32 = 1,
	Ray64 = 2,
	Ray96 = 3,
	Ray128 = 4
};

enum SettingSpriteDensity
{
	LowSprites = 0,
	MediumSprites = 1,
	HighSprites = 2,
	VeryHighSprites = 3
};

/*
	In this function put settings.add* lines to set what the default settings should be
	These will be written to the text file if it's called in-between an import and export
*/
void populateDefaults(std::shared_ptr<SettingManager> settings);
