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
	PreferenceFloat = 3,
	PreferenceColor = 4 //aka vec4
};

class SettingManager;
struct PreferenceNode;

/*
	Value key pair that stores a preference
*/
struct PreferencePair
{
	friend struct PreferenceNode;
	friend class SettingManager;

	//Only used in DrawSettingsWindow, transfers to string value upon export
	int valueInt = 0;
	//Only used in DrawSettingsWindow, transfers to string value upon export
	float valueFloat = 0;
	//Only used in DrawSettingsWindow, transfers to string value upon export
	bool valueBool = false;
	//Only used in DrawSettingsWindow, both floats and ints
	float maxValue = 0, minValue = 100;
	//Used for color preferences only
	float color[4] = { 1,1,1,1 };

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
	/*
 		Only used if created or changed with SettingManager::addEnum
   		An actual C++ enum should be created alongside each one of these
     */
	std::vector<std::string> dropDownNames;
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
	//Full path including name
	std::string path = "";
	//Lower level branches of the tree
	std::vector<PreferenceNode*> childNodes;
	//Leaves of the tree
	std::vector<PreferencePair*> childPreferences;

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

		//Used in nextPreferenceBinding cleared in startPreferenceBindingSearch
		int preferenceSearchIndex = 0;
		//Used in nextPreferenceBinding cleared in startPreferenceBindingSearch
		int nodeSearchIndex = 0;

		//So we can do non-recursive searches
		std::vector<PreferenceNode*> allNodes;

	public:

		/*
			Call startPreferenceBindingSearch once first, then call this in a loop
			Returns a pointer to the data held in a preference, or nullptr when the loop should end
			Name and type describe the preference whose value pointer is being returned
			Path does not include name
		*/
		PreferencePair *nextPreferenceBinding(std::string& path);

		//Call one between looping through nextPreferenceBinding
		void startPreferenceBindingSearch();

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
		PreferencePair /*const* const*/* getPreference(std::string path) const;

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
			will return 1,1,1,1 if preference doens't exist
		*/
		glm::vec4 getColor(std::string path) const;

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
		void addInt(std::string path, int value, bool override = true,std::string desc = "",float min=0,float max=100);

		//Creates a preference, will create nodes along the way if needed
		void addString(std::string path, std::string value, bool override = true,std::string desc = "");

		//Creates a preference, will create nodes along the way if needed
		void addBool(std::string path, bool value, bool override = true,std::string desc = "");

		//Creates a preference, will create nodes along the way if needed
		void addFloat(std::string path, float value,bool override = true,std::string desc = "", float min = 0, float max = 100);

		//Creates a preference, will create nodes along the way if needed
		void addColor(std::string path, glm::vec4 value, bool override = true, std::string desc = "");

		//Returns any preference as a string, or "" if no preference exists at that path
		std::string operator[] (std::string path) const;

		/*
			Mostly an integer preference under the hood, differs only on how settings UI will be rendered
			A C++ enum should be created for use alongside any enum preference to convert its values
   			Will not overwrite existing values
		*/
		void addEnum(std::string path, int value,std::string desc,std::vector<std::string> &&names);
};
