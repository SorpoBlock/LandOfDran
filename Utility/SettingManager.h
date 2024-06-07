#pragma once

#include "../LandOfDran.h"

/*
	The four valid types of preferences
*/
enum PreferenceType
{
	PreferenceString = 0,
	PreferenceBoolean = 1,
	PreferenceInteger = 2,
	PreferenceFloat = 3
};

class SettingManager;

/*
	Value key pair that stores a preference
*/
struct PreferencePair
{
	//What kind of value should the string be converted to
	PreferenceType type = PreferenceString;
	//The setting this value represents, case insensitive
	std::string name = "";
	//The value is read as a string but can be converted to boolean, integer, or float
	std::string value = "";
	/*
 		Used for GUI generation in the settings menu, if string is empty ("") no GUI element will be created
   		Not saved to text files, set in DefaultPreferences.cpp
     	*/
	std::string description = "";
};

/*
	Can have one or more preferences or one or more PreferenceNodes
*/
struct PreferenceNode
{
	friend class SettingManager;

	//Will be nullptr for rootNode, and valid for all other nodes
	PreferenceNode* parent = nullptr;
	//The name of this node in the tree
	std::string name = "";
	//Lower level branches of the tree
	std::vector<PreferenceNode> childNodes;
	//Leaves of the tree
	std::vector<PreferencePair> childPreferences;

	private:
		//For recursive writing, level determines how many tabs to start line with
		void writeToFile(std::ofstream& file, int level = 0) const;

		//Ignoring initial tabs, reads a preference from a line of a preference file
		void readFromLine(std::string& line, int lineNumber, PreferenceNode* currentNode);
};

/*
	Reads a simple text file and creates a hierarchical tree of preferences
	preferences can be strings, booleans, integers, or floats,
	names are case-insensitive
*/
class SettingManager
{
	private:
		//Base of the preferences tree, start traversal here
		PreferenceNode rootNode;

		//Writes a warning to users to be careful of editing preference files manually
		static void writePreferenceWarningToFile(std::ofstream& file);

	public:
		//Load the settings/preferences from a text file
		SettingManager(std::string path);

		//Save the preferences contained within to a file, comments will be lost
		void exportToFile(std::string path) const;
		/*
			Get a preference by specifying a path to a preference
			i.e. AvatarPreferences/NodeColors/LeftHand
			Use / and never \
			Can return nullptr on bad path
		*/
		PreferencePair const * const getPreference(std::string path) const;

		/*
			Gets a preference by specifiying a path to a preference
			i.e. AvatarPreferences/NodeColors/LeftHand
			Use / and never \
			will return 0 if preference doens't exist
		*/
		float getFloat(std::string path) const;

		/*
			Gets a preference by specifiying a path to a preference
			i.e. AvatarPreferences/NodeColors/LeftHand
			Use / and never \
			will return "" if preference doens't exist
		*/
		std::string getString(std::string path) const;

		/*
			Gets a preference by specifiying a path to a preference
			i.e. AvatarPreferences/NodeColors/LeftHand
			Use / and never \
			will return false if preference doens't exist
		*/
		bool getBool(std::string path) const;

		/*
			Gets a preference by specifiying a path to a preference
			i.e. AvatarPreferences/NodeColors/LeftHand
			Use / and never \
			will return 0 if preference doens't exist
		*/
		int getInt(std::string path) const;

		/*
			Creates nodes on the way to the path
			Sets the path to the final preference's name
			returns the final node created
		*/
		PreferenceNode *createPathTo(std::string &path);

		//Creates a preference, will create nodes along the way if needed
		void addInt(std::string path, int value, bool override = true);

		//Creates a preference, will create nodes along the way if needed
		void addString(std::string path, std::string value, bool override = true);

		//Creates a preference, will create nodes along the way if needed
		void addBool(std::string path, bool value, bool override = true);

		//Creates a preference, will create nodes along the way if needed
		void addFloat(std::string path, float value,bool override = true);

		//Returns any preference as a string, or "" if no preference exists at that path
		std::string operator[] (std::string path) const;
};
