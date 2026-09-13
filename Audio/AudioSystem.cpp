#include "AudioSystem.h"
#include "ReverbPresets.h"
#include "SoundFile.h"
#include "../SimObjects/Dynamic.h"

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>
#include <AL/efx.h>

//How many meters one world unit (a stud) sounds like, sets how long echoes take to come back
static constexpr float metersPerUnit = 0.5f;
static constexpr float speedOfSound = 343.0f;

//Auto reverb eases toward what the space calls for over roughly this long, and is sent to OpenAL this often
static constexpr float reverbSmoothingMS = 400.0f;
static constexpr float reverbApplyIntervalMS = 50.0f;

//Low-pass filter amounts for a completely blocked sound, and for being underwater, multiplied together when both apply
static constexpr float occludedGain = 0.7f;
static constexpr float occludedGainHF = 0.15f;
static constexpr float underwaterGain = 0.7f;
static constexpr float underwaterGainHF = 0.1f;
static constexpr float occlusionSmoothingMS = 80.0f;
static constexpr float underwaterFadeMS = 250.0f;

static const EFXEAXREVERBPROPERTIES underwaterReverb = EFX_REVERB_PRESET_UNDERWATER;

//Where it's drawn, or where its body is if it hasn't been drawn yet (the drawn position starts at the origin and glides over)
static glm::vec3 soundPositionOf(const Dynamic& dynamic)
{
	return dynamic.renderedTransformInitialized ? dynamic.renderedPosition : b2g3(dynamic.getPosition());
}

SoundLocation SoundLocation::at(const glm::vec3& position)
{
	SoundLocation ret;
	ret.kind = Fixed;
	ret.position = position;
	return ret;
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

const btRigidBody* SoundLocation::body() const
{
	if (kind != Attached)
		return nullptr;

	std::shared_ptr<Dynamic> target = dynamic.lock();
	return target ? target->body : nullptr;
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

/*
	Reverb for a space with surfaces averageDistance away (world units) that closes in enclosure (0-1) of the way around
	Bigger spaces echo longer and later, closed in spaces echo louder, and nearby walls give strong early reflections
	Out in the open there's barely any
*/
static EFXEAXREVERBPROPERTIES reverbForSpace(float averageDistance, float enclosure)
{
	EFXEAXREVERBPROPERTIES reverb = EFX_REVERB_PRESET_GENERIC;

	float size = std::max(averageDistance * metersPerUnit, 0.1f);
	float closedIn = std::clamp(enclosure, 0.0f, 1.0f);

	reverb.flDecayTime = std::clamp((0.3f + size * 0.12f) * (0.4f + 0.6f * closedIn), 0.1f, 20.0f);
	reverb.flReflectionsDelay = std::clamp(size * 2.0f / speedOfSound, 0.0f, 0.3f);
	reverb.flLateReverbDelay = std::clamp(size / speedOfSound + 0.005f, 0.0f, 0.1f);
	reverb.flGain = 0.02f + 0.3f * closedIn * std::sqrt(closedIn);
	reverb.flReflectionsGain = std::clamp(0.05f + 1.2f * closedIn * std::clamp(6.0f / size, 0.2f, 1.0f), 0.0f, 3.16f);
	reverb.flLateReverbGain = 1.2589f * (0.3f + 0.7f * closedIn);
	reverb.flDensity = std::clamp(0.3f + size / 15.0f, 0.3f, 1.0f);

	return reverb;
}

//Moves every parameter of into amount (0-1) of the way to toward
static void blendReverb(EFXEAXREVERBPROPERTIES& into, const EFXEAXREVERBPROPERTIES& toward, float amount)
{
	auto blend = [amount](float& value, float target) { value += (target - value) * amount; };

	blend(into.flDensity, toward.flDensity);
	blend(into.flDiffusion, toward.flDiffusion);
	blend(into.flGain, toward.flGain);
	blend(into.flGainHF, toward.flGainHF);
	blend(into.flGainLF, toward.flGainLF);
	blend(into.flDecayTime, toward.flDecayTime);
	blend(into.flDecayHFRatio, toward.flDecayHFRatio);
	blend(into.flDecayLFRatio, toward.flDecayLFRatio);
	blend(into.flReflectionsGain, toward.flReflectionsGain);
	blend(into.flReflectionsDelay, toward.flReflectionsDelay);
	blend(into.flLateReverbGain, toward.flLateReverbGain);
	blend(into.flLateReverbDelay, toward.flLateReverbDelay);
	blend(into.flEchoTime, toward.flEchoTime);
	blend(into.flEchoDepth, toward.flEchoDepth);
	blend(into.flModulationTime, toward.flModulationTime);
	blend(into.flModulationDepth, toward.flModulationDepth);
	blend(into.flAirAbsorptionGainHF, toward.flAirAbsorptionGainHF);
	blend(into.flHFReference, toward.flHFReference);
	blend(into.flLFReference, toward.flLFReference);
	blend(into.flRoomRolloffFactor, toward.flRoomRolloffFactor);
	for (int a = 0; a < 3; a++)
	{
		blend(into.flReflectionsPan[a], toward.flReflectionsPan[a]);
		blend(into.flLateReverbPan[a], toward.flLateReverbPan[a]);
	}

	if (amount >= 0.5f)
		into.iDecayHFLimit = toward.iDecayHFLimit;
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

	ALint slot = on ? (ALint)effectSlot : AL_EFFECTSLOT_NULL;

	for (int a = 0; a < generalSourceCount; a++)
		alSource3i(generalSources[a], AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL);
	for (int a = 0; a < loopSourceCount; a++)
		alSource3i(loopSources[a], AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL);
}

void AudioSystem::updateReverbConnection()
{
	connectReverb(reverbMode == ReverbPreset || (reverbMode == ReverbAuto && environmentalReverb));
}

void AudioSystem::applyReverb(const EFXEAXREVERBPROPERTIES& reverb)
{
	if (!effect || !effectSlot)
		return;

	if (useEaxReverb)
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

void AudioSystem::applyDirectFilter(ALuint source, Occlusion& occlusion)
{
	if (!directFilter)
		return;

	float gain = glm::mix(1.0f, occludedGain, occlusion.current) * glm::mix(1.0f, underwaterGain, underwaterAmount);
	float gainHF = glm::mix(1.0f, occludedGainHF, occlusion.current) * glm::mix(1.0f, underwaterGainHF, underwaterAmount);

	if (std::abs(gain - occlusion.appliedGain) < 0.003f && std::abs(gainHF - occlusion.appliedGainHF) < 0.003f)
		return;

	//Like effect slots, a source copies the filter's settings when it's attached
	alFilterf(directFilter, AL_LOWPASS_GAIN, gain);
	alFilterf(directFilter, AL_LOWPASS_GAINHF, gainHF);
	alSourcei(source, AL_DIRECT_FILTER, (ALint)directFilter);

	occlusion.appliedGain = gain;
	occlusion.appliedGainHF = gainHF;
}

void AudioSystem::startOcclusion(ALuint source, Occlusion& occlusion, const SoundLocation& where)
{
	occlusion = Occlusion();

	if (occlusionOn && occlusionTest && where.kind != SoundLocation::Flat)
	{
		occlusion.target = occlusionTest(listenerPosition, where.position, where.body());
		occlusion.current = occlusion.target;
	}

	applyDirectFilter(source, occlusion);
}

void AudioSystem::updateOcclusion(ALuint source, Occlusion& occlusion, const SoundLocation& where, float deltaT)
{
	if (occlusionOn && occlusionTest && where.kind != SoundLocation::Flat)
	{
		occlusion.sinceCheckMS += deltaT;
		if (occlusion.sinceCheckMS >= occlusionIntervalMS)
		{
			occlusion.sinceCheckMS = 0;
			occlusion.target = occlusionTest(listenerPosition, where.position, where.body());
		}
	}
	else
		occlusion.target = 0;

	occlusion.current += (occlusion.target - occlusion.current) * (1.0f - std::exp(-deltaT / occlusionSmoothingMS));
	applyDirectFilter(source, occlusion);
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
	startOcclusion(source, generalOcclusion[index], generalLocations[index]);

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

	if (isAutoReverb(preset))
	{
		reverbMode = ReverbAuto;
		updateReverbConnection();
		return;
	}

	if (isNoReverb(preset))
	{
		reverbMode = ReverbOff;
		updateReverbConnection();
		return;
	}

	EFXEAXREVERBPROPERTIES reverb;
	if (!getReverbPreset(normalizeReverbPreset(preset), reverb))
	{
		error("Unknown reverb preset " + preset);
		return;
	}

	reverbMode = ReverbPreset;
	currentReverb = reverb;
	applyReverb(reverb);
	updateReverbConnection();

	ALenum alError = alGetError();
	if (alError != AL_NO_ERROR)
		error("OpenAL error " + std::to_string(alError) + " setting reverb preset " + preset);
}

void AudioSystem::setEnvironmentOptions(int reverbQuality, int occlusionQuality)
{
	static constexpr float intervalsMS[3] = { 250.0f, 150.0f, 100.0f };

	environmentalReverb = reverbQuality > 0;
	occlusionOn = occlusionQuality > 0;
	occlusionIntervalMS = intervalsMS[std::clamp(occlusionQuality, 1, 3) - 1];

	if (valid)
		updateReverbConnection();
}

void AudioSystem::setListenerSpace(float averageDistance, float enclosure)
{
	spaceDistance = averageDistance;
	spaceEnclosure = enclosure;
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

void AudioSystem::update(const glm::vec3& position, const glm::vec3& listenerDirection, float deltaT)
{
	if (!valid)
		return;

	listenerPosition = position;
	alListener3f(AL_POSITION, position.x, position.y, position.z);
	alListener3f(AL_VELOCITY, 0, 0, 0);

	//Up is world up tilted to be perpendicular to where the camera looks
	glm::vec3 forward = glm::length(listenerDirection) > 0.0001f ? glm::normalize(listenerDirection) : glm::vec3(0, 0, -1);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
	glm::vec3 up = glm::length(right) > 0.0001f ? glm::normalize(glm::cross(right, forward)) : glm::vec3(0, 0, -1);
	ALfloat orientation[6] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
	alListenerfv(AL_ORIENTATION, orientation);

	underwaterAmount = std::clamp(underwaterAmount + (underwater ? 1.0f : -1.0f) * deltaT / underwaterFadeMS, 0.0f, 1.0f);

	//Reverb glides toward what the space around the listener calls for
	if (wantsListenerSpace())
	{
		EFXEAXREVERBPROPERTIES target = reverbForSpace(spaceDistance, spaceEnclosure);
		blendReverb(target, underwaterReverb, underwaterAmount);
		blendReverb(currentReverb, target, 1.0f - std::exp(-deltaT / reverbSmoothingMS));

		sinceReverbAppliedMS += deltaT;
		if (sinceReverbAppliedMS >= reverbApplyIntervalMS)
		{
			sinceReverbAppliedMS = 0;
			applyReverb(currentReverb);
		}
	}

	//One-shot sounds: follow the Dynamic they're on (staying where it was last if it's deleted), and muffling
	for (int a = 0; a < generalSourceCount; a++)
	{
		ALint state;
		alGetSourcei(generalSources[a], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			generalLocations[a] = SoundLocation::flat();
			continue;
		}

		if (generalLocations[a].kind == SoundLocation::Attached && generalLocations[a].follow())
			alSource3f(generalSources[a], AL_POSITION, generalLocations[a].position.x, generalLocations[a].position.y, generalLocations[a].position.z);

		updateOcclusion(generalSources[a], generalOcclusion[a], generalLocations[a], deltaT);
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
	auto distanceSquared = [&position](const Loop& loop)
	{
		if (loop.where.kind == SoundLocation::Flat)
			return 0.0f;
		glm::vec3 offset = loop.where.position - position;
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
			updateOcclusion(loopSources[loop.source], loopOcclusion[loop.source], loop.where, deltaT);
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
			startOcclusion(source, loopOcclusion[b], loop.where);
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

	reverbMode = ReverbAuto;

	if (valid)
	{
		for (int a = 0; a < generalSourceCount; a++)
		{
			alSourceStop(generalSources[a]);
			alSourcei(generalSources[a], AL_BUFFER, 0);
			generalLocations[a] = SoundLocation::flat();
		}

		updateReverbConnection();

		for (SoundType& sound : sounds)
			if (sound.buffer)
				alDeleteBuffers(1, &sound.buffer);
	}

	sounds.clear();
}

AudioSystem::AudioSystem()
{
	scope("AudioSystem::AudioSystem");

	currentReverb = reverbForSpace(spaceDistance, spaceEnclosure);

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
		useEaxReverb = alGetEnumValue("AL_EFFECT_EAXREVERB") != 0;

		alError = alGetError();
		if (alError != AL_NO_ERROR)
		{
			error("OpenAL error " + std::to_string(alError) + " creating reverb, reverb won't do anything");
			effect = 0;
			effectSlot = 0;
		}

		alGenFilters(1, &directFilter);
		alFilteri(directFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);

		alError = alGetError();
		if (alError != AL_NO_ERROR)
		{
			error("OpenAL error " + std::to_string(alError) + " creating a low-pass filter, sounds won't be muffled");
			directFilter = 0;
		}
	}
	else
		info("OpenAL implementation has no EFX, reverb and muffling won't do anything");

	const ALCchar* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
	info("Audio started on " + std::string(deviceName ? deviceName : "unknown device"));

	valid = true;

	applyReverb(currentReverb);
	updateReverbConnection();
}

AudioSystem::~AudioSystem()
{
	if (!valid)
		return;

	clear();

	for (int a = 0; a < generalSourceCount; a++)
		alSourcei(generalSources[a], AL_DIRECT_FILTER, AL_FILTER_NULL);
	for (int a = 0; a < loopSourceCount; a++)
		alSourcei(loopSources[a], AL_DIRECT_FILTER, AL_FILTER_NULL);

	alDeleteSources(generalSourceCount, generalSources);
	alDeleteSources(loopSourceCount, loopSources);

	if (directFilter)
		alDeleteFilters(1, &directFilter);
	if (effectSlot)
		alDeleteAuxiliaryEffectSlots(1, &effectSlot);
	if (effect)
		alDeleteEffects(1, &effect);

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}
