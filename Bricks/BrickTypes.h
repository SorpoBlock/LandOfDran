#pragma once

#include "../LandOfDran.h"

#include <unordered_set>

//A named basic brick size, only needed because Blockland saves store bricks by name rather than size
struct BasicBrickType
{
	//As written in types.txt and .bls saves, e.g. "2x4" or "1x1F"
	std::string uiName = "";

	unsigned char width = 1;
	unsigned char height = 1;
	unsigned char length = 1;

	//Icon image next to the type's .blb, "" if there isn't one
	std::string iconPath = "";
};

/*
	Brick names from a types folder like Assets/brick/types: test.cs maps names to .blb files, and each .blb's
	first two lines give its size and whether it's a basic box or a special brick
*/
class BrickTypes
{
	std::vector<BasicBrickType> basicTypes;

	//Lowercase name to index in basicTypes
	std::unordered_map<std::string, size_t> basicByName;

	//Lowercase names of special bricks, which can't be loaded yet
	std::unordered_set<std::string> specialNames;

	public:

	void load(const std::string& typesFolder);

	/*
		Size of a basic brick by name, case-insensitive. Names without a .blb fall back to Blockland's naming:
		"2x4" is one brick (3 plates) tall, "1x2F" is one plate tall, "1x1x5" is 5 bricks tall
		Returns false for special and unknown bricks
	*/
	bool getBasicSize(const std::string& name, int& width, int& height, int& length) const;

	const std::vector<BasicBrickType>& getBasicTypes() const { return basicTypes; }
};
