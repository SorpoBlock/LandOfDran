#include "AudioSystem.h"
#include "ReverbPresets.h"
#include "SoundFile.h"
#include "../SimObjects/Dynamic.h"

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>

SoundLocation SoundLocation::at(const glm::vec3& position)
{
	SoundLocation ret;
	ret.kind = Fixed;
	ret.position = position;
	return ret;
}

//Where it's drawn, or where its body is if it hasn't been drawn yet (the drawn position starts at the origin and glides over)
static glm::vec3 soundPositionOf(const Dynamic& dynamic)
{
	return dynamic.renderedTransformInitialized ? dynamic.renderedPosition : b2g3(dynamic.getPosition());
}

SoundLocation SoundLocation::on(const std::shared_ptr<Dynamic>& dynamic)
{
	SoundLocation ret;
	ret.kind = Attached;
	ret.dynamic = dynamic;
	if (dynamic)
		ret.position = soundPositionOf(*dynamic);
	return ret;
}

bool SoundLocation::follow()
{
	if (kind != Attached)
		return true;

	std::shared_ptr<Dynamic> target = dynamic.lock();
	if (!target)
		return false;

	position = soundPositionOf(*target);
	return true;
}

//The old client's list, see ReverbPresets.h for the names
static bool getReverbPreset(const std::string& name, EFXEAXREVERBPROPERTIES& result)
{
	static const std::map<std::string, EFXEAXREVERBPROPERTIES> presets =
	{
		{ "generic",		EFX_REVERB_PRESET_GENERIC },
		{ "paddedcell",		EFX_REVERB_PRESET_PADDEDCELL },
		{ "auditorium",		EFX_REVERB_PRESET_AUDITORIUM },
		{ "concerthall",	EFX_REVERB_PRESET_CONCERTHALL },
		{ "cave",			EFX_REVERB_PRESET_CAVE },
		{ "forest",			EFX_REVERB_PRESET_FOREST },
		{ "plain",			EFX_REVERB_PRESET_PLAIN },
		{ "underwater",		EFX_REVERB_PRESET_UNDERWATER },
		{ "drugged",		EFX_REVERB_PRESET_DRUGGED },
		{ "dizzy",			EFX_REVERB_PRESET_DIZZY },
		{ "psychotic",		EFX_REVERB_PRESET_PSYCHOTIC },
		{ "outhouse",		EFX_REVERB_PRESET_PREFAB_OUTHOUSE },
		{ "heaven",			EFX_REVERB_PRESET_MOOD_HEAVEN },
		{ "hell",			EFX_REVERB_PRESET_MOOD_HELL },
		{ "memory",			EFX_REVERB_PRESET_MOOD_MEMORY },
		{ "dustyroom",		EFX_REVERB_PRESET_DUSTYROOM },
		{ "waterroom",		EFX_REVERB_PRESET_SMALLWATERROOM },
		{ "racer",			EFX_REVERB_PRESET_DRIVING_INCAR_RACER },
		{ "tunnel",			EFX_REVERB_PRESET_DRIVING_TUNNEL }
	};

	auto found = presets.find(name);
	if (found == presets.end())
		return false;

	result = found->second;
	return true;
}

void AudioSystem::placeSource(ALuint source, const SoundLocation& where)
{
	bool positioned = where.kind != SoundLocation::Flat;
	glm::vec3 position = positioned ? where.position : glm::vec3(0);

	//Relative to the listener at 0,0,0 is how OpenAL plays something with no direction
	alSourcei(source, AL_SOURCE_RELATIVE, positioned ? AL_FALSE : AL_TRUE);
	alSource3f(source, AL_POSITION, position.x, position.y, position.z);
	alSource3f(source, AL_VELOCITY, 0, 0, 0);

	if (canSpatializeStereo)
		alSourcei(source, AL_SOURCE_SPATIALIZE_SOFT, positioned ? AL_TRUE : AL_FALSE);
}

void AudioSystem::connectReverb(bool on)
{
	if (!effectSlot)
		return;

	reverbOn = on;
	ALint slot = on ? (ALint)effectSlot : AL_EFFECTSLOT_NULL;

	for (int a = 0; a < generalSourceCount; a++)
		alSource3i(generalSources[a], AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL);
	for (int a = 0; a < loopSourceCount; a++)
		alSource3i(loopSources[a], AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL);
}

void AudioSystem::stopLoopSource(Loop& loop, bool rememberOffset)
{
	if (loop.source == -1)
		return;

	ALuint source = loopSources[loop.source];
	if (rememberOffset)
		alGetSourcei(source, AL_SAMPLE_OFFSET, &loop.sampleOffset);
	else
		loop.sampleOffset = 0;

	alSourceStop(source);
	alSourcei(source, AL_BUFFER, 0);

	loopSourceUsed[loop.source] = false;
	loop.source = -1;
}

void AudioSystem::addSoundType(int id, const std::string& name, const std::string& filePath, bool isMusic)
{
	scope("AudioSystem::addSoundType");

	if (id < 0)
		return;

	if ((size_t)id >= sounds.size())
		sounds.resize(id + 1);

	SoundType& sound = sounds[id];

	//The server can send the whole list and a newly added sound at the same time to someone who's just joining
	if (sound.name == name && sound.filePath == filePath && sound.isMusic == isMusic)
		return;

	if (sound.buffer)
	{
		//OpenAL won't delete a buffer that a source still has
		for (int a = 0; a < generalSourceCount; a++)
		{
			ALint attached;
			alGetSourcei(generalSources[a], AL_BUFFER, &attached);
			if ((ALuint)attached != sound.buffer)
				continue;

			alSourceStop(generalSources[a]);
			alSourcei(generalSources[a], AL_BUFFER, 0);
		}

		for (Loop& loop : loops)
			if (loop.soundID == id)
				stopLoopSource(loop, false);

		alDeleteBuffers(1, &sound.buffer);
		sound.buffer = 0;
	}

	sound.name = name;
	sound.filePath = filePath;
	sound.isMusic = isMusic;

	if (!valid)
		return;

	//File paths come from the server, don't let one reach outside the game's folder
	std::filesystem::path path(filePath);
	bool outside = path.is_absolute() || path.has_root_name() || path.has_root_directory();
	for (const std::filesystem::path& part : path)
		outside = outside || part == "..";

	if (outside)
	{
		error("Not loading sound " + name + ", its file isn't inside the game folder: " + filePath);
		return;
	}

	std::vector<int16_t> samples;
	int channels = 0, sampleRate = 0;
	if (!decodeSoundFile(filePath, samples, channels, sampleRate))
		return;

	alGenBuffers(1, &sound.buffer);
	alBufferData(sound.buffer, channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, samples.data(), (ALsizei)(samples.size() * sizeof(int16_t)), sampleRate);

	ALenum alError = alGetError();
	if (alError != AL_NO_ERROR)
	{
		error("OpenAL error " + std::to_string(alError) + " loading sound " + filePath);
		alDeleteBuffers(1, &sound.buffer);
		sound.buffer = 0;
		return;
	}

	debug("Loaded sound " + name + " from " + filePath);
}

int AudioSystem::findSound(const std::string& name) const
{
	for (size_t a = 0; a < sounds.size(); a++)
		if (sounds[a].name == name)
			return (int)a;

	return -1;
}

void AudioSystem::playSound(int soundID, const SoundLocation& where, float pitch, float volume)
{
	if (!valid || soundID < 0 || (size_t)soundID >= sounds.size() || !sounds[soundID].buffer)
		return;

	//Find a source that's done playing, starting after the one used last
	int index = lastUsedGeneralSource;
	bool foundFree = false;
	for (int searched = 0; searched < generalSourceCount; searched++)
	{
		index = (index + 1) % generalSourceCount;

		ALint state;
		alGetSourcei(generalSources[index], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			foundFree = true;
			break;
		}
	}

	//All of them are busy, cut off the one that's gone longest since being picked
	if (!foundFree)
		index = (lastUsedGeneralSource + 1) % generalSourceCount;

	ALuint source = generalSources[index];
	alSourceStop(source);
	alSourcei(source, AL_BUFFER, sounds[soundID].buffer);
	alSourcei(source, AL_LOOPING, AL_FALSE);
	alSourcef(source, AL_PITCH, std::clamp(pitch, 0.05f, 10.0f));
	alSourcef(source, AL_GAIN, std::clamp(volume, 0.0f, 1.0f));

	generalLocations[index] = where;
	generalLocations[index].follow();
	placeSource(source, generalLocations[index]);

	alSourcePlay(source);
	lastUsedGeneralSource = index;

	ALenum alError = alGetError();
	if (alError != AL_NO_ERROR)
		error("OpenAL error " + std::to_string(alError) + " playing sound " + sounds[soundID].name);
}

void AudioSystem::playSound(const std::string& name, const SoundLocation& where, float pitch, float volume)
{
	int soundID = findSound(name);
	if (soundID != -1)
		playSound(soundID, where, pitch, volume);
}

void AudioSystem::startLoop(unsigned int id, int soundID, const SoundLocation& where, float pitch, float volume)
{
	if (stoppedBeforeStarting.erase(id))
		return;

	//Same ID again replaces it
	for (auto loop = loops.begin(); loop != loops.end(); ++loop)
	{
		if (loop->id != id)
			continue;

		stopLoopSource(*loop, false);
		loops.erase(loop);
		break;
	}

	Loop loop;
	loop.id = id;
	loop.soundID = soundID;
	loop.where = where;
	loop.where.follow();
	loop.pitch = std::clamp(pitch, 0.05f, 10.0f);
	loop.volume = std::clamp(volume, 0.0f, 1.0f);
	loops.push_back(loop);

	//It gets a source, if it's close enough, in update
}

void AudioSystem::stopLoop(unsigned int id)
{
	for (auto loop = loops.begin(); loop != loops.end(); ++loop)
	{
		if (loop->id != id)
			continue;

		stopLoopSource(*loop, false);
		loops.erase(loop);
		return;
	}

	stoppedBeforeStarting.insert(id);
}

void AudioSystem::setReverb(const std::string& preset)
{
	scope("AudioSystem::setReverb");

	if (!valid)
		return;

	if (isNoReverb(preset))
	{
		connectReverb(false);
		return;
	}

	EFXEAXREVERBPROPERTIES reverb;
	if (!getReverbPreset(normalizeReverbPreset(preset), reverb))
	{
		error("Unknown reverb preset " + preset);
		return;
	}

	//This OpenAL has no EFX, see the constructor
	if (!effect || !effectSlot)
		return;

	if (alGetEnumValue("AL_EFFECT_EAXREVERB") != 0)
	{
		alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
		alEffectf(effect, AL_EAXREVERB_DENSITY, reverb.flDensity);
		alEffectf(effect, AL_EAXREVERB_DIFFUSION, reverb.flDiffusion);
		alEffectf(effect, AL_EAXREVERB_GAIN, reverb.flGain);
		alEffectf(effect, AL_EAXREVERB_GAINHF, reverb.flGainHF);
		alEffectf(effect, AL_EAXREVERB_GAINLF, reverb.flGainLF);
		alEffectf(effect, AL_EAXREVERB_DECAY_TIME, reverb.flDecayTime);
		alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, reverb.flDecayHFRatio);
		alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, reverb.flDecayLFRatio);
		alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain);
		alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay);
		alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, reverb.flReflectionsPan);
		alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain);
		alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay);
		alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, reverb.flLateReverbPan);
		alEffectf(effect, AL_EAXREVERB_ECHO_TIME, reverb.flEchoTime);
		alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, reverb.flEchoDepth);
		alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, reverb.flModulationTime);
		alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, reverb.flModulationDepth);
		alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF);
		alEffectf(effect, AL_EAXREVERB_HFREFERENCE, reverb.flHFReference);
		alEffectf(effect, AL_EAXREVERB_LFREFERENCE, reverb.flLFReference);
		alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor);
		alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit);
	}
	else
	{
		alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
		alEffectf(effect, AL_REVERB_DENSITY, reverb.flDensity);
		alEffectf(effect, AL_REVERB_DIFFUSION, reverb.flDiffusion);
		alEffectf(effect, AL_REVERB_GAIN, reverb.flGain);
		alEffectf(effect, AL_REVERB_GAINHF, reverb.flGainHF);
		alEffectf(effect, AL_REVERB_DECAY_TIME, reverb.flDecayTime);
		alEffectf(effect, AL_REVERB_DECAY_HFRATIO, reverb.flDecayHFRatio);
		alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain);
		alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay);
		alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain);
		alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay);
		alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF);
		alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor);
		alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit);
	}

	//A slot copies the effect's settings when it's attached, so it has to be attached again after every change
	alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, (ALint)effect);
	connectReverb(true);

	ALenum alError = alGetError();
	if (alError != AL_NO_ERROR)
		error("OpenAL error " + std::to_string(alError) + " setting reverb preset " + preset);
}

void AudioSystem::setVolumes(float master, float music)
{
	musicVolume = std::clamp(music, 0.0f, 1.0f);

	if (!valid)
		return;

	alListenerf(AL_GAIN, std::clamp(master, 0.0f, 1.0f));

	for (const Loop& loop : loops)
		if (loop.source != -1)
			alSourcef(loopSources[loop.source], AL_GAIN, loop.volume * musicVolume);
}

void AudioSystem::update(const glm::vec3& listenerPosition, const glm::vec3& listenerDirection)
{
	if (!valid)
		return;

	alListener3f(AL_POSITION, listenerPosition.x, listenerPosition.y, listenerPosition.z);
	alListener3f(AL_VELOCITY, 0, 0, 0);

	//Up is world up tilted to be perpendicular to where the camera looks
	glm::vec3 forward = glm::length(listenerDirection) > 0.0001f ? glm::normalize(listenerDirection) : glm::vec3(0, 0, -1);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
	glm::vec3 up = glm::length(right) > 0.0001f ? glm::normalize(glm::cross(right, forward)) : glm::vec3(0, 0, -1);
	ALfloat orientation[6] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
	alListenerfv(AL_ORIENTATION, orientation);

	//One-shot sounds following a Dynamic, they stay where it was last if it's deleted
	for (int a = 0; a < generalSourceCount; a++)
	{
		if (generalLocations[a].kind != SoundLocation::Attached)
			continue;

		ALint state;
		alGetSourcei(generalSources[a], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			generalLocations[a] = SoundLocation::flat();
			continue;
		}

		if (generalLocations[a].follow())
			alSource3f(generalSources[a], AL_POSITION, generalLocations[a].position.x, generalLocations[a].position.y, generalLocations[a].position.z);
	}

	//A loop on a Dynamic ends with it, the server forgets it the same way
	for (auto loop = loops.begin(); loop != loops.end();)
	{
		if (loop->where.follow())
		{
			++loop;
			continue;
		}

		stopLoopSource(*loop, false);
		loop = loops.erase(loop);
	}

	//Closest loops first, loops with no position count as right on top of the listener
	auto distanceSquared = [&listenerPosition](const Loop& loop)
	{
		if (loop.where.kind == SoundLocation::Flat)
			return 0.0f;
		glm::vec3 offset = loop.where.position - listenerPosition;
		return glm::dot(offset, offset);
	};

	//Stable so loops the same distance away don't keep trading sources every frame
	std::stable_sort(loops.begin(), loops.end(), [&distanceSquared](const Loop& a, const Loop& b) { return distanceSquared(a) < distanceSquared(b); });

	//Too far, pause them where they are to pick up there later
	for (size_t a = loopSourceCount; a < loops.size(); a++)
		stopLoopSource(loops[a], true);

	for (size_t a = 0; a < std::min(loops.size(), (size_t)loopSourceCount); a++)
	{
		Loop& loop = loops[a];

		if (loop.source != -1)
		{
			if (loop.where.kind != SoundLocation::Flat)
				alSource3f(loopSources[loop.source], AL_POSITION, loop.where.position.x, loop.where.position.y, loop.where.position.z);
			continue;
		}

		if (loop.soundID < 0 || (size_t)loop.soundID >= sounds.size() || !sounds[loop.soundID].buffer)
			continue;

		//Only loopSourceCount loops ever get this far, so there's always a free source
		for (int b = 0; b < loopSourceCount; b++)
		{
			if (loopSourceUsed[b])
				continue;

			loopSourceUsed[b] = true;
			loop.source = b;

			ALuint source = loopSources[b];
			alSourcei(source, AL_BUFFER, sounds[loop.soundID].buffer);
			alSourcei(source, AL_LOOPING, AL_TRUE);
			alSourcef(source, AL_PITCH, loop.pitch);
			alSourcef(source, AL_GAIN, loop.volume * musicVolume);
			placeSource(source, loop.where);
			alSourcei(source, AL_SAMPLE_OFFSET, loop.sampleOffset);
			alSourcePlay(source);
			break;
		}
	}

	ALenum alError = alGetError();
	if (alError != AL_NO_ERROR)
		error("OpenAL error " + std::to_string(alError) + " updating audio");
}

void AudioSystem::clear()
{
	stoppedBeforeStarting.clear();

	for (Loop& loop : loops)
		stopLoopSource(loop, false);
	loops.clear();

	if (valid)
	{
		for (int a = 0; a < generalSourceCount; a++)
		{
			alSourceStop(generalSources[a]);
			alSourcei(generalSources[a], AL_BUFFER, 0);
			generalLocations[a] = SoundLocation::flat();
		}

		connectReverb(false);

		for (SoundType& sound : sounds)
			if (sound.buffer)
				alDeleteBuffers(1, &sound.buffer);
	}

	sounds.clear();
}

AudioSystem::AudioSystem()
{
	scope("AudioSystem::AudioSystem");

	device = alcOpenDevice(nullptr);
	if (!device)
	{
		error("Could not open an audio device, there won't be any sound");
		return;
	}

	context = alcCreateContext(device, nullptr);
	if (!context || !alcMakeContextCurrent(context))
	{
		error("Could not create an OpenAL context, there won't be any sound");
		if (context)
			alcDestroyContext(context);
		alcCloseDevice(device);
		context = nullptr;
		device = nullptr;
		return;
	}

	alGenSources(generalSourceCount, generalSources);
	alGenSources(loopSourceCount, loopSources);

	ALenum alError = alGetError();
	if (alError != AL_NO_ERROR)
	{
		error("OpenAL error " + std::to_string(alError) + " creating sources, there won't be any sound");
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);
		context = nullptr;
		device = nullptr;
		return;
	}

	canSpatializeStereo = alIsExtensionPresent("AL_SOFT_source_spatialize");

	if (alcIsExtensionPresent(device, "ALC_EXT_EFX"))
	{
		alGenEffects(1, &effect);
		alGenAuxiliaryEffectSlots(1, &effectSlot);

		alError = alGetError();
		if (alError != AL_NO_ERROR)
		{
			error("OpenAL error " + std::to_string(alError) + " creating reverb, reverb presets won't do anything");
			effect = 0;
			effectSlot = 0;
		}
	}
	else
		info("OpenAL implementation has no EFX, reverb presets won't do anything");

	const ALCchar* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
	info("Audio started on " + std::string(deviceName ? deviceName : "unknown device"));

	valid = true;
}

AudioSystem::~AudioSystem()
{
	if (!valid)
		return;

	clear();

	alDeleteSources(generalSourceCount, generalSources);
	alDeleteSources(loopSourceCount, loopSources);

	if (effectSlot)
		alDeleteAuxiliaryEffectSlots(1, &effectSlot);
	if (effect)
		alDeleteEffects(1, &effect);

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}
