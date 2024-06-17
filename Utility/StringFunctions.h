#pragma once

#include "../LandOfDran.h"


//Used for taking the result of a sha256 has and turning it into a string in hexidecimal 
std::string GetHexRepresentation(const unsigned char* Bytes, size_t Length);

/*
	Returns a lowercase version of a string
	stripOtherChars will remove non alpha-numeric characters
*/
std::string lowercase(std::string in, bool stripOtherChars = false);

// remove all white space from a string, including spaces, new lines, and tabs
std::string& removeAllWhitespace(std::string& s);

// trim whitespace from left side
std::string leftTrim(std::string& s);

//Replace all instances of from with to in a string
void replaceAll(std::string& source, const std::string& from, const std::string& to);