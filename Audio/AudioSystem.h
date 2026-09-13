#pragma once

#include "../LandOfDran.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx-presets.h>

#include <set>

//Not included, ClientProgramData includes this and Dynamic's headers lead back to ClientProgramData
class Dynamic;
class btRigidBody;

//Where a sound plays from: nowhere in particular (same volume in both ears), a fixed point, or following a Dynamic
struct SoundLocation
{
	enum Kind
	{
		Flat,
		Fixed,
		Attached
	} kind = Flat;

	glm::vec3 position = glm::vec3(0);
	std::weak_ptr<Dynamic> dynamic;

	static SoundLocation flat() { return SoundLocation(); }
	static SoundLocation at(const glm::vec3& position);
	static SoundLocation on(const std::shared_ptr<Dynamic>& dynamic);

	//Moves position to where an attached Dynamic is drawn, false once that Dynamic is gone (position stays where it was last)
	bool follow();

	//The physics body of the Dynamic it follows, nullptr if it doesn't follow one
	const btRigidBody* body() const;
};

/*
	Client only: plays the sounds the server registered, through OpenAL
	Ported from the old client's audioPlayer: a pool of sources for one-shot sounds, and a smaller pool for looping sounds
	where only the loops closest to the listener get one, the rest pausing until they're close enough again
	On top of the old client: reverb that follows the space around the listener, muffling underwater, and muffling
	sounds with something in the way (occlusion)
	Lives for the whole program, clear() forgets everything from the server we just left
*/
class AudioSystem
{
public:

	static constexpr int generalSourceCount = 64;
	static constexpr int loopSourceCount = 16;

	/*
		0 when nothing is between the listener and a sound, up to 1 when it's completely blocked, see AcousticProbe::occlusion
		sourceBody is the body of a Dynamic the sound follows, if any, which shouldn't count as blocking it
	*/
	typedef std::function<float(const glm::vec3& listener, const glm::vec3& source, const btRigidBody* sourceBody)> OcclusionTest;

private:

	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	bool valid = false;

	//AL_SOFT_source_spatialize, lets stereo sounds have a position too (OpenAL leaves stereo unpositioned otherwise)
	bool canSpatializeStereo = false;

	struct SoundType
	{
		std::string name = "";
		std::string filePath = "";
		bool isMusic = false;
		//0 if the file couldn't be loaded, playing it does nothing
		ALuint buffer = 0;
	};

	//Index is the ID the server gave the sound
	std::vector<SoundType> sounds;

	//How muffled a source is by something in the way
	struct Occlusion
	{
		float target = 0;
		float current = 0;
		float sinceCheckMS = 0;
		//What the source's low-pass filter was last set to, -1 to make sure it gets set
		float appliedGain = -1;
		float appliedGainHF = -1;
	};

	ALuint generalSources[generalSourceCount] = {};
	SoundLocation generalLocations[generalSourceCount];
	Occlusion generalOcclusion[generalSourceCount];
	//Where the search for a free source starts, so a burst of sounds doesn't keep checking the same busy ones
	int lastUsedGeneralSource = 0;

	struct Loop
	{
		unsigned int id = 0;
		int soundID = -1;
		SoundLocation where;
		float pitch = 1.0f;
		float volume = 1.0f;
		//Index into loopSources, -1 while it's too far away to have one
		int source = -1;
		//Where playback left off when it lost its source, so it picks up there later
		ALint sampleOffset = 0;
	};

	std::vector<Loop> loops;
	ALuint loopSources[loopSourceCount] = {};
	bool loopSourceUsed[loopSourceCount] = {};
	Occlusion loopOcclusion[loopSourceCount];

	//Stop requests for loops we haven't started yet (a start waiting on its Dynamic to arrive), so the start is dropped instead
	std::set<unsigned int> stoppedBeforeStarting;

	//Loops are music bricks, engines and ambience, so they use the music volume on top of their own
	float musicVolume = 0.5f;

	//EFX reverb and low-pass filter, all 0 if the OpenAL implementation doesn't have ALC_EXT_EFX
	ALuint effect = 0;
	ALuint effectSlot = 0;
	ALuint directFilter = 0;
	bool useEaxReverb = false;

	//Auto follows the listener's space and water (see setListenerSpace), Preset holds one Lua picked, Off has none
	enum ReverbMode
	{
		ReverbAuto,
		ReverbPreset,
		ReverbOff
	} reverbMode = ReverbAuto;

	//What the reverb slot has, in auto it glides toward what the space calls for
	EFXEAXREVERBPROPERTIES currentReverb;
	float sinceReverbAppliedMS = 0;

	//Settings, see setEnvironmentOptions
	bool environmentalReverb = true;
	bool occlusionOn = true;
	float occlusionIntervalMS = 150.0f;
	OcclusionTest occlusionTest = nullptr;

	float spaceDistance = 40.0f;
	float spaceEnclosure = 0.0f;
	bool underwater = false;
	//0-1, fades in and out instead of switching all at once
	float underwaterAmount = 0.0f;

	glm::vec3 listenerPosition = glm::vec3(0);

	//Sets the source up to play from where, including whether it's positioned at all
	void placeSource(ALuint source, const SoundLocation& where);

	//Routes every source through the reverb slot, or takes them all off it
	void connectReverb(bool on);

	//Reverb is on for a preset, and in auto when environmental reverb is turned on
	void updateReverbConnection();

	void applyReverb(const EFXEAXREVERBPROPERTIES& reverb);

	void stopLoopSource(Loop& loop, bool rememberOffset);

	//For a source that just started playing where: checks right away, so a sound behind a wall doesn't start out clear
	void startOcclusion(ALuint source, Occlusion& occlusion, const SoundLocation& where);

	//Rechecks every occlusionIntervalMS, glides toward the result, and sets the source's filter from it and underwaterAmount
	void updateOcclusion(ALuint source, Occlusion& occlusion, const SoundLocation& where, float deltaT);

	void applyDirectFilter(ALuint source, Occlusion& occlusion);

public:

	bool isValid() const { return valid; }

	//Loads the file right away. A second packet for the same ID replaces it, unless it's the same sound again
	void addSoundType(int id, const std::string& name, const std::string& filePath, bool isMusic);

	//-1 if the server hasn't registered a sound with that name
	int findSound(const std::string& name) const;

	void playSound(int soundID, const SoundLocation& where, float pitch = 1.0f, float volume = 1.0f);

	//For sounds the client plays on its own, like brick clicks. Silent if the server didn't register one by that name
	void playSound(const std::string& name, const SoundLocation& where = SoundLocation::flat(), float pitch = 1.0f, float volume = 1.0f);

	//id is chosen by the server, and used to stop the loop later
	void startLoop(unsigned int id, int soundID, const SoundLocation& where, float pitch, float volume);
	void stopLoop(unsigned int id);

	//"auto" follows the listener's space, "none" turns reverb off, or a name from reverbPresetNames
	void setReverb(const std::string& preset);

	//audio/reverbquality and audio/occlusionquality: 0 off, 1-3 low to high (occlusion quality sets how often it's rechecked)
	void setEnvironmentOptions(int reverbQuality, int occlusionQuality);

	void setOcclusionTest(const OcclusionTest& test) { occlusionTest = test; }

	//Whether reverb currently follows setListenerSpace, so there's no need to measure the space otherwise
	bool wantsListenerSpace() const { return valid && effectSlot && reverbMode == ReverbAuto && environmentalReverb; }

	//From AcousticProbe::measure
	void setListenerSpace(float averageDistance, float enclosure);

	//Muffles everything and switches auto reverb toward the underwater preset
	void setUnderwater(bool isUnderwater) { underwater = isUnderwater; }

	//Both 0-1, master scales everything, music scales loops
	void setVolumes(float master, float music);

	//Call every frame: moves the listener, follows moving sounds, updates reverb and muffling, and hands loop sources to the closest loops
	void update(const glm::vec3& listenerPosition, const glm::vec3& listenerDirection, float deltaT);

	//Stops everything and forgets the server's sounds, loops, and reverb preset, for leaving a server
	void clear();

	AudioSystem();
	~AudioSystem();
};
