#pragma once

#include "../LandOfDran.h"
#include "../External/CRC.h"

/*
	Scripts and add-ons aren't meant to be able to access/edit files outside the LoD directory
	Or certain files that come with LoD itself
*/
bool okayFilePath(const std::string &path);

/*
	CRC32 checksum of the file at path
*/
unsigned int getFileChecksum(const char* filePath);

/*
	Gets filesize in bytes, 0 if file invalid
*/
long GetFileSize(const std::string &filename);

/*
	Gets the file name and extension from a full file path
	e.g. "assets/bob/bob.png" as filepath returns "bob.png"
*/
std::string getFileFromPath(const std::string &in);

/*
	Gets the folder from a full file path
	e.g. "assets/bob/bob.png" as filepath returns "assets/bob/"
*/
std::string getFolderFromPath(const std::string &in);

/*
	Adds a suffix to the end of a file name before the extension
	E.g. in = "test/bob.png", suffix = "_a" will return "test/bob_a.png"
*/
std::string addSuffixToFile(std::string in, std::string suffix);

bool doesFileExist(const std::string &filePath);