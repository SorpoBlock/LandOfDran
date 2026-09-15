#pragma once

#include <string>
#include <vector>
#include <cstdint>

/*
	Decodes a .wav (any format dr_wav reads) , .ogg (Vorbis), or .mp3 file to interleaved signed 16 bit samples
	Only mono and stereo files are accepted. Returns false and logs why if the file couldn't be used
*/
bool decodeSoundFile(const std::string& filePath, std::vector<int16_t>& samples, int& channels, int& sampleRate);
