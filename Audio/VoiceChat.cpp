#include "VoiceChat.h"

#include <opus/opus.h>
#include <cstring>

//Carries a 16 bit sequence number on from the full sequence number it's closest to, so it keeps counting past 65535
static int64_t unwrapSequence(int64_t newest, uint16_t sequence)
{
	int16_t difference = (int16_t)(uint16_t)(sequence - (uint16_t)newest);
	return newest + difference;
}

std::vector<std::string> VoiceChat::listMicrophones()
{
	std::vector<std::string> names = { "Default" };

	//A double null terminated list
	const ALCchar* list = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);
	for (const ALCchar* name = list; name && *name; name += strlen(name) + 1)
		names.push_back(name);

	return names;
}

//"Default" in any case, or nothing at all
static bool isSystemDefault(const std::string& microphoneName)
{
	return microphoneName.empty() || lowercase(microphoneName) == "default";
}

bool VoiceChat::openMicrophone()
{
	scope("VoiceChat::openMicrophone");

	if (microphone)
		return true;

	if (microphoneFailed)
		return false;

	if (!encoder)
	{
		int opusError = OPUS_OK;
		encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, &opusError);
		if (opusError != OPUS_OK || !encoder)
		{
			error("Could not create an Opus encoder, voice chat can't send: " + std::string(opus_strerror(opusError)));
			encoder = nullptr;
			microphoneFailed = true;
			return false;
		}

		opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
		opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
		//Each frame carries a rougher copy of the one before it, so one lost packet can be rebuilt from the next
		opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));
		opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(10));
	}

	//Room for half a second, far more than piles up between two frames
	microphone = alcCaptureOpenDevice(isSystemDefault(microphoneName) ? nullptr : microphoneName.c_str(), sampleRate, AL_FORMAT_MONO16, sampleRate / 2);
	if (!microphone)
	{
		std::vector<std::string> microphones = listMicrophones();
		std::string names = "";
		for (size_t a = 1; a < microphones.size(); a++)
			names += (a > 1 ? ", " : "") + microphones[a];

		error("Could not open microphone \"" + microphoneName + "\", voice chat can't send. Microphones: " + (names.empty() ? "none" : names));
		microphoneFailed = true;
		return false;
	}

	openedMicrophoneName = microphoneName;
	const ALCchar* openedName = alcGetString(microphone, ALC_CAPTURE_DEVICE_SPECIFIER);
	info("Voice chat recording from " + std::string(openedName ? openedName : "unknown microphone"));
	return true;
}

void VoiceChat::closeMicrophone()
{
	if (!microphone)
		return;

	if (recording)
		alcCaptureStop(microphone);
	recording = false;

	alcCaptureCloseDevice(microphone);
	microphone = nullptr;
}

void VoiceChat::record(bool sending, const SendFrame& send, float deltaT)
{

	//A changed audio/microphone setting takes effect between transmissions
	if (microphone && !recording && openedMicrophoneName != microphoneName)
		closeMicrophone();

	if (sending && !recording && openMicrophone())
	{
		//Whatever's left from last time was recorded before push to talk was pressed
		ALCint stale = 0;
		alcGetIntegerv(microphone, ALC_CAPTURE_SAMPLES, 1, &stale);
		if (stale > 0)
		{
			std::vector<int16_t> discard(stale);
			alcCaptureSamples(microphone, discard.data(), stale);
		}

		opus_encoder_ctl(encoder, OPUS_RESET_STATE);
		alcCaptureStart(microphone);
		recording = true;
	}

	if (!recording)
	{
		if (microphone)
		{
			sinceRecordedMS += deltaT;
			if (sinceRecordedMS >= closeMicrophoneAfterMS)
				closeMicrophone();
		}
		return;
	}

	sinceRecordedMS = 0;
	inputLevel *= std::exp(-deltaT / levelReleaseMS);

	opus_int16 samples[frameSamples];
	unsigned char compressed[maxVoiceFrameBytes];

	ALCint available = 0;
	alcGetIntegerv(microphone, ALC_CAPTURE_SAMPLES, 1, &available);
	while (available >= frameSamples)
	{
		alcCaptureSamples(microphone, samples, frameSamples);
		available -= frameSamples;

		if (!sending)
			continue;

		if (microphoneVolume != 1.0f)
			for (opus_int16& sample : samples)
				sample = (opus_int16)std::clamp(sample * microphoneVolume, -32768.0f, 32767.0f);

		//The frame's loudness for the meter, as RMS in dB below full scale
		double sumOfSquares = 0;
		bool clipped = false;
		for (opus_int16 sample : samples)
		{
			sumOfSquares += (double)sample * sample;
			clipped = clipped || sample == 32767 || sample == -32768;
		}
		float loudnessDB = 10.0f * (float)std::log10(sumOfSquares / frameSamples / (32768.0 * 32768.0) + 1e-10);
		inputLevel = std::max(inputLevel, std::clamp(1.0f - loudnessDB / levelFloorDB, 0.0f, 1.0f));
		if (clipped)
		{
			lastClipMS = SDL_GetTicks();
			clippedAny = true;
		}

		opus_int32 length = opus_encode(encoder, samples, frameSamples, compressed, maxVoiceFrameBytes);
		if (length < 0)
		{
			error("Opus couldn't encode voice: " + std::string(opus_strerror(length)));
			continue;
		}

		send(0, nextSequence, compressed, (unsigned int)length);
		nextSequence++;
		transmitting = true;
	}

	if (sending)
		return;

	alcCaptureStop(microphone);
	recording = false;

	if (transmitting)
	{
		//Takes no sequence number, so the next transmission carries straight on from this one
		send(VoiceFlag_End, nextSequence, nullptr, 0);
		transmitting = false;
	}
}

void VoiceChat::play(Talker& talker)
{
	if (!audio->isValid())
		return;

	unsigned int now = SDL_GetTicks();

	if (talker.voice == -1)
	{
		if (talker.waiting.empty())
			return;

		talker.voice = audio->openVoice(talker.where);
		if (talker.voice == -1)
		{
			//Every voice source is taken, keep just the newest few frames so they don't start out behind once one frees up
			while (talker.waiting.size() > prebufferFrames)
				talker.waiting.erase(talker.waiting.begin());
			return;
		}
	}

	audio->moveVoice(talker.voice, talker.where);
	int queued = audio->queuedVoiceBuffers(talker.voice);

	if (talker.buffering)
	{
		if (talker.waiting.empty())
		{
			//Nothing left to play and they've gone quiet, give the source back for someone else
			if (queued == 0 && now - talker.lastPacketMS > releaseSourceMS)
			{
				audio->closeVoice(talker.voice);
				talker.voice = -1;
			}
			return;
		}

		if (talker.waiting.size() < prebufferFrames && !talker.ended && now - talker.bufferingSinceMS < prebufferMS)
			return;

		talker.buffering = false;
		talker.nextSequence = talker.waiting.begin()->first;
	}

	while (talker.waiting.size() > maxWaitingFrames)
		talker.waiting.erase(talker.waiting.begin());

	opus_int16 samples[frameSamples];
	while (queued < maxQueuedFrames)
	{
		//Too many missing in a row to fill in, skip ahead to what's there
		if (!talker.waiting.empty() && talker.waiting.begin()->first - talker.nextSequence > maxConcealedFrames)
			talker.nextSequence = talker.waiting.begin()->first;

		int decoded = 0;
		auto next = talker.waiting.find(talker.nextSequence);
		if (next != talker.waiting.end())
		{
			decoded = opus_decode(talker.decoder, next->second.data(), (opus_int32)next->second.size(), samples, frameSamples, 0);
			talker.waiting.erase(next);
		}
		else if (talker.waiting.empty() || queued > lowQueuedFrames)
			break; //Nothing to play yet, or there's still time left to play before the missing one has to be filled in
		else
		{
			//Lost: rebuilt from the copy in the frame after it if that's here, otherwise Opus makes up something that fits
			auto after = talker.waiting.find(talker.nextSequence + 1);
			if (after != talker.waiting.end())
				decoded = opus_decode(talker.decoder, after->second.data(), (opus_int32)after->second.size(), samples, frameSamples, 1);
			else
				decoded = opus_decode(talker.decoder, nullptr, 0, samples, frameSamples, 0);
		}

		talker.nextSequence++;

		if (decoded < 0)
		{
			error("Opus couldn't decode voice: " + std::string(opus_strerror(decoded)));
			continue;
		}

		audio->queueVoice(talker.voice, samples, decoded, sampleRate);
		queued++;
	}

	//Ran dry, wait for a few frames again rather than stuttering along one at a time
	if (talker.waiting.empty() && audio->queuedVoiceBuffers(talker.voice) == 0)
		talker.buffering = true;
}

void VoiceChat::forget(Talker& talker)
{
	if (talker.voice != -1)
		audio->closeVoice(talker.voice);
	talker.voice = -1;

	if (talker.decoder)
		opus_decoder_destroy(talker.decoder);
	talker.decoder = nullptr;
}

void VoiceChat::setMicrophone(const std::string& name, float volume)
{
	if (name != microphoneName)
		microphoneFailed = false;

	microphoneName = name;
	microphoneVolume = std::clamp(volume, 0.0f, 4.0f);
}

void VoiceChat::update(bool pushToTalk, const SendFrame& send, float deltaT)
{
	bool held = pushToTalk && !muted;
	sinceReleasedMS = held ? 0 : sinceReleasedMS + deltaT;

	record(held || (transmitting && !muted && sinceReleasedMS < releaseTailMS), send, deltaT);

	unsigned int now = SDL_GetTicks();
	for (auto talker = talkers.begin(); talker != talkers.end();)
	{
		play(talker->second);

		if (talker->second.voice == -1 && talker->second.waiting.empty() && now - talker->second.lastPacketMS > forgetTalkerMS)
		{
			forget(talker->second);
			talker = talkers.erase(talker);
			continue;
		}

		++talker;
	}
}

void VoiceChat::receive(netIDType id, unsigned char flags, uint16_t sequence, const SoundLocation& where, const unsigned char* data, unsigned int length)
{
	scope("VoiceChat::receive");

	if (!audio->isValid())
		return;

	Talker& talker = talkers[id];
	if (!talker.decoder)
	{
		int opusError = OPUS_OK;
		talker.decoder = opus_decoder_create(sampleRate, 1, &opusError);
		if (opusError != OPUS_OK || !talker.decoder)
		{
			error("Could not create an Opus decoder, can't play someone's voice: " + std::string(opus_strerror(opusError)));
			talkers.erase(id);
			return;
		}
	}

	unsigned int now = SDL_GetTicks();
	talker.where = where;
	talker.lastPacketMS = now;

	if (flags & VoiceFlag_End)
	{
		talker.ended = true;
		return;
	}

	if (length == 0)
		return;

	int64_t fullSequence = talker.heardAny ? unwrapSequence(talker.newestSequence, sequence) : sequence;

	if (talker.buffering && talker.waiting.empty())
	{
		//Starting after a pause, whatever came before isn't going to be played
		talker.nextSequence = fullSequence;
		talker.bufferingSinceMS = now;
		opus_decoder_ctl(talker.decoder, OPUS_RESET_STATE);
	}
	else if (fullSequence < talker.nextSequence)
		return; //Too late, it was already filled in or skipped

	talker.ended = false;
	talker.heardAny = true;
	talker.newestSequence = std::max(talker.newestSequence, fullSequence);
	talker.waiting[fullSequence].assign(data, data + length);
}

void VoiceChat::setTalkerName(netIDType talker, const std::string& name)
{
	talkerNames[talker] = name;
}

std::vector<std::string> VoiceChat::getSpeaking() const
{
	std::vector<std::string> names;

	for (const auto& [id, talker] : talkers)
	{
		if (talker.voice == -1 || (talker.waiting.empty() && audio->queuedVoiceBuffers(talker.voice) == 0))
			continue;

		auto name = talkerNames.find(id);
		names.push_back(name != talkerNames.end() ? name->second : "Someone");
	}

	return names;
}

void VoiceChat::clear()
{
	closeMicrophone();
	microphoneFailed = false;
	transmitting = false;
	sinceReleasedMS = 0;
	muted = false;

	for (auto& [id, talker] : talkers)
		forget(talker);
	talkers.clear();
	talkerNames.clear();
}

VoiceChat::VoiceChat(std::shared_ptr<AudioSystem> _audio) : audio(_audio)
{

}

VoiceChat::~VoiceChat()
{
	clear();

	if (encoder)
		opus_encoder_destroy(encoder);
}
