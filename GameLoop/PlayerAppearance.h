#pragma once

#include "../LandOfDran.h"

/*
	How a player wants their player model to look, picked in the appearance editor (see Interface/AppearanceEditor.h)
	Their game sends it to the server as they connect (see Networking/PacketsFromClient/AppearanceChoice.cpp)
	and the server keeps it until Lua puts it on a player with client:applyAppearance
*/
struct PlayerAppearance
{
	//File name of an image in Assets/faces, empty for no face
	std::string face = "";

	//Mesh names, lower case like settings keep them, and the color each part is painted
	std::vector<std::pair<std::string, glm::vec3>> colors;

	//Longest face or mesh name sent either way
	static constexpr unsigned int maxNameLength = 64;

	//Most painted parts one appearance sends
	static constexpr unsigned int maxColors = 64;
};
