#include "SoundFile.h"

#include "../LandOfDran.h"

#define DR_WAV_IMPLEMENTATION
#include "../External/dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "../External/dr_mp3.h"

//Just the declarations, the implementation is compiled in StbVorbis.cpp
#define STB_VORBIS_HEADER_ONLY
#include "../External/stb_vorbis.c"

bool decodeSoundFile(const std::string& filePath, std::vector<int16_t>& samples, int& channels, int& sampleRate)
{
	scope("decodeSoundFile");

	std::string extension = "";
	size_t dot = filePath.find_last_of('.');
	if (dot != std::string::npos)
		extension = lowercase(filePath.substr(dot + 1));

	if (extension == "wav")
	{
		unsigned int wavChannels = 0, wavSampleRate = 0;
		drwav_uint64 frames = 0;
		drwav_int16* data = drwav_open_file_and_read_pcm_frames_s16(filePath.c_str(), &wavChannels, &wavSampleRate, &frames, nullptr);
		if (!data)
		{
			error("Could not read wav file " + filePath);
			return false;
		}

		channels = (int)wavChannels;
		sampleRate = (int)wavSampleRate;
		samples.assign(data, data + frames * wavChannels);
		drwav_free(data, nullptr);
	}
	else if (extension == "ogg")
	{
		short* data = nullptr;
		int frames = stb_vorbis_decode_filename(filePath.c_str(), &channels, &sampleRate, &data);
		if (frames < 0 || !data)
		{
			error("Could not read ogg file " + filePath);
			return false;
		}

		samples.assign(data, data + (size_t)frames * channels);
		free(data);
	}
	else if (extension == "mp3")
	{
		drmp3_config config;
		drmp3_uint64 frames = 0;
		drmp3_int16* data = drmp3_open_file_and_read_pcm_frames_s16(filePath.c_str(), &config, &frames, nullptr);
		if (!data)
		{
			error("Could not read mp3 file " + filePath);
			return false;
		}

		channels = (int)config.channels;
		sampleRate = (int)config.sampleRate;
		samples.assign(data, data + frames * config.channels);
		drmp3_free(data, nullptr);
	}
	else
	{
		error(filePath + ": unsupported audio format, use .wav, .ogg, or .mp3");
		return false;
	}

	if (channels != 1 && channels != 2)
	{
		error(filePath + " has " + std::to_string(channels) + " channels, only mono and stereo sounds are supported");
		samples.clear();
		return false;
	}

	if (samples.empty())
	{
		error(filePath + " has no audio in it");
		return false;
	}

	return true;
}
