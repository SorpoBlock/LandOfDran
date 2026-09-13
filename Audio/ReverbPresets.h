#pragma once

#include <string>
#include <algorithm>

/*
	Names accepted by setAudioEffect, shared so the server can reject a typo before sending it
	The client maps each to an OpenAL EFX preset in AudioSystem.cpp, same list as the old client
*/
inline constexpr const char* reverbPresetNames[] =
{
	"generic", "paddedcell", "auditorium", "concerthall", "cave", "forest", "plain", "underwater", "drugged", "dizzy",
	"psychotic", "outhouse", "heaven", "hell", "memory", "dustyroom", "waterroom", "racer", "tunnel"
};

//Lower case, so names are case insensitive
inline std::string normalizeReverbPreset(std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return name;
}

//"none" (or empty, or the old client's "default" and "normal") turns reverb off
inline bool isNoReverb(const std::string& name)
{
	std::string lower = normalizeReverbPreset(name);
	return lower == "" || lower == "none" || lower == "default" || lower == "normal";
}

inline bool isReverbPreset(const std::string& name)
{
	if (isNoReverb(name))
		return true;

	std::string lower = normalizeReverbPreset(name);
	for (const char* preset : reverbPresetNames)
		if (lower == preset)
			return true;

	return false;
}
