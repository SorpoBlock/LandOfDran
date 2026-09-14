#pragma once

#include "BrickHolder.h"
#include "BrickTypes.h"

/*
	Save files live directly in the Saves folder
	Returns the full path for a save file name, or "" if the name could reach outside that folder
*/
std::string getSavePath(const std::string& fileName);

/*
	Old Land of Dran binary save format, written like OldServer/lua/miscFunctions.h's saveBuild except that each brick's
	music, light, and emitter are written as BrickAttachments under a newer version number the old game can't read
	Returns false if the file couldn't be written
*/
bool saveLodBuild(const BrickHolder& bricks, const std::string& path, bool omitOwnership);

/*
	Loads either version of the old binary format or our own newer one, offset by whole studs/plates
	Special bricks of types we don't have, and the old game's lights, music, and prints, are skipped
	Returns how many bricks were added, or -1 if the file couldn't be read
*/
int loadLodBuild(BrickHolder& bricks, const std::string& path, int offsetX, int offsetY, int offsetZ);

/*
	How a Blockland import finds our versions of what a .bls save put on its bricks, by the uiName the save uses
	findEmitterType and findMusic return the name of our type, or "" if we don't have one
	setLight gives attachments our light for a Blockland light type, false if we don't have one
*/
struct BlocklandAttachmentLookup
{
	std::function<bool(const std::string&, BrickAttachments&)> setLight = nullptr;
	std::function<std::string(const std::string&)> findEmitterType = nullptr;
	std::function<std::string(const std::string&)> findMusic = nullptr;
};

/*
	Imports a Blockland .bls save, using the file's own color palette
	Bricks are resolved by name through types, special and unknown bricks are skipped and logged by name
	Brick lights, emitters, and music go through lookup, and any we don't have are skipped and logged by name
	Returns how many bricks were added, or -1 if the file couldn't be read
*/
int loadBlocklandBuild(BrickHolder& bricks, const BrickTypes& types, const std::string& path, const BlocklandAttachmentLookup& lookup);
