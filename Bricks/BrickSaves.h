#pragma once

#include "BrickHolder.h"
#include "BrickTypes.h"

/*
	Save files live directly in the Saves folder
	Returns the full path for a save file name, or "" if the name could reach outside that folder
*/
std::string getSavePath(const std::string& fileName);

/*
	Old Land of Dran binary save format, written exactly like OldServer/lua/miscFunctions.h's saveBuild
	Returns false if the file couldn't be written
*/
bool saveLodBuild(const BrickHolder& bricks, const std::string& path, bool omitOwnership);

/*
	Loads either version of the old binary format, offset by whole studs/plates
	Special bricks, lights, music, and prints in the file are skipped
	Returns how many bricks were added, or -1 if the file couldn't be read
*/
int loadLodBuild(BrickHolder& bricks, const std::string& path, int offsetX, int offsetY, int offsetZ);

/*
	Imports a Blockland .bls save, using the file's own color palette
	Bricks are resolved by name through types, special and unknown bricks are skipped and logged by name
	Returns how many bricks were added, or -1 if the file couldn't be read
*/
int loadBlocklandBuild(BrickHolder& bricks, const BrickTypes& types, const std::string& path);
