#pragma once

#include "../LandOfDran.h"
#include "AudioSystem.h"

//From opus.h, which only VoiceChat.cpp needs
typedef struct OpusEncoder OpusEncoder;
typedef struct OpusDecoder OpusDecoder;

/*
	Client only: proximity voice chat
	Records the microphone while push to talk is held, compresses it with Opus 20 ms at a time, and hands each frame off to be sent to the server
	The server passes frames on to everyone close enough along with where the talker is, and here they're decoded into an AudioSystem voice
	source on the talker's player, so voices get quieter with distance, muffled through walls, and echo like any other sound
	Frames can arrive late or not at all, so each talker's frames wait in a short buffer, and Opus fills in the ones that were lost
*/
class VoiceChat
{
public:

	static constexpr int sampleRate = 48000;
	//20 ms
	static constexpr int frameSamples = 960;

	//Called with each compressed frame to send, see makeVoiceFramePacket. Empty with VoiceFlag_End when a transmission ends
	typedef std::function<void(unsigned char flags, uint16_t sequence, const unsigned char* data, unsigned int length)> SendFrame;

private:

	std::shared_ptr<AudioSystem> audio;

	//Bits per second, plenty for speech
	static constexpr int bitrate = 24000;

	//How long recording carries on after push to talk is let go, so the end of the last word isn't cut off
	static constexpr float releaseTailMS = 200.0f;
	//An open microphone that hasn't recorded for this long is closed, so the system doesn't show it in use the whole time
	static constexpr float closeMicrophoneAfterMS = 30000.0f;

	//A talker starts playing once this many frames are waiting, or the first has waited prebufferMS. How much that queues up is how far
	//behind its arrival everything after plays, which is what covers frames arriving unevenly
	static constexpr size_t prebufferFrames = 4;
	static constexpr unsigned int prebufferMS = 80;
	//Frames are decoded into OpenAL as they arrive, up to maxQueuedFrames. Past that they wait, and past maxWaitingFrames the oldest are dropped,
	//so a burst of late packets doesn't leave a talker behind for good
	static constexpr int maxQueuedFrames = 8;
	static constexpr size_t maxWaitingFrames = 4;
	//A missing frame with later ones already here is lost, it's filled in once no more than lowQueuedFrames are left to play
	static constexpr int lowQueuedFrames = 1;
	//Past this many missing in a row, skip ahead instead of filling them all in
	static constexpr int maxConcealedFrames = 5;
	//A quiet talker gives their voice source back after releaseSourceMS, and is forgotten after forgetTalkerMS
	static constexpr unsigned int releaseSourceMS = 1000;
	static constexpr unsigned int forgetTalkerMS = 10000;

	ALCdevice* microphone = nullptr;
	//audio/microphone, "Default" (any case) for the system's default. The open microphone is reopened between transmissions when it changes
	std::string microphoneName = "Default";
	std::string openedMicrophoneName = "";
	//Don't try to open it again every frame after it failed, until the setting changes or we leave the server
	bool microphoneFailed = false;
	float microphoneVolume = 1.0f;
	float sinceRecordedMS = 0;

	OpusEncoder* encoder = nullptr;

	//Microphone meter, see getInputLevel. Rises right away and falls over roughly levelReleaseMS, a frame levelFloorDB below full scale or quieter reads as empty
	float inputLevel = 0;
	static constexpr float levelReleaseMS = 150.0f;
	static constexpr float levelFloorDB = -50.0f;
	//SDL_GetTicks of the last frame with a sample at the limit of 16 bits, isClipping stays on for clipHoldMS after
	unsigned int lastClipMS = 0;
	bool clippedAny = false;
	static constexpr unsigned int clipHoldMS = 300;

	bool recording = false;
	bool transmitting = false;
	float sinceReleasedMS = 0;
	uint16_t nextSequence = 0;

	//The server muted us, see client:setVoiceMuted
	bool muted = false;

	struct Talker
	{
		OpusDecoder* decoder = nullptr;
		//From AudioSystem::openVoice, -1 while it doesn't have one
		int voice = -1;
		SoundLocation where;

		//Frames that arrived and haven't been played yet, by sequence number counted on past 16 bits
		std::map<int64_t, std::vector<unsigned char>> waiting;
		int64_t newestSequence = 0;
		int64_t nextSequence = 0;
		bool heardAny = false;

		//Waiting for enough frames to start, or start again after running dry. Since bufferingSinceMS
		bool buffering = true;
		unsigned int bufferingSinceMS = 0;

		//Got VoiceFlag_End, the next frame starts a new transmission
		bool ended = false;
		unsigned int lastPacketMS = 0;
	};

	std::map<netIDType, Talker> talkers;
	std::map<netIDType, std::string> talkerNames;

	bool openMicrophone();
	void closeMicrophone();

	//Records while sending, and sends each full frame
	void record(bool sending, const SendFrame& send, float deltaT);

	//Decodes waiting frames into the talker's voice source to keep it fed, filling in lost ones
	void play(Talker& talker);

	void forget(Talker& talker);

public:

	//"Default" then the name of every microphone OpenAL can record from, for audio/microphone
	static std::vector<std::string> listMicrophones();

	//audio/microphone and audio/microphonevolume (0-4)
	void setMicrophone(const std::string& name, float volume);

	void setMuted(bool isMuted) { muted = isMuted; }
	bool isMuted() const { return muted; }

	//Whether our voice is going out right now
	bool isTransmitting() const { return transmitting; }

	//How loud the microphone is while transmitting, after microphone volume: 0 for silence to 1 for full scale
	float getInputLevel() const { return transmitting ? inputLevel : 0.0f; }

	//Whether the microphone just hit the loudest 16 bit audio can hold while transmitting, so the voice going out is distorted
	bool isClipping() const { return transmitting && clippedAny && SDL_GetTicks() - lastClipMS < clipHoldMS; }

	//Call every frame: records and sends while pushToTalk is held (and a moment after), and plays what's arrived from others
	void update(bool pushToTalk, const SendFrame& send, float deltaT);

	//A VoiceFrameFromServer packet's contents, where is where the talker is
	void receive(netIDType talker, unsigned char flags, uint16_t sequence, const SoundLocation& where, const unsigned char* data, unsigned int length);

	void setTalkerName(netIDType talker, const std::string& name);

	//Names of the people who can be heard talking right now
	std::vector<std::string> getSpeaking() const;

	//Stops recording and playback and forgets every talker, for leaving a server
	void clear();

	VoiceChat(std::shared_ptr<AudioSystem> audio);
	~VoiceChat();
};
