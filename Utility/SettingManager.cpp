#include "SettingManager.h"

void SettingManager::addEnum(std::string path, int value,std::vector<std::string> &&names)
{
	PreferenceNode *target = createPathTo(path);

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a].name == path)
		{
			target->description = desc;
			target->enumnames = std::move(names);
			return;
		}
	}

	target->childPreferences.emplace_back();
	target->childPreferences.back().value = std::to_string(value);
	target->childPreferences.back().type = PreferenceInteger;
	target->childPreferences.back().name = path;
	target->description = desc;
	target->enumnames = std::move(names);
}

PreferenceNode* SettingManager::createPathTo(std::string &path)
{
	//Why would this ever happen?
	if (path.length() == 0)
		return nullptr;

	//Strip final slashes because you shouldn't put those in anyway
	if (path[path.length() - 1] == '/')
		path = path.substr(0, path.length() - 1);

	PreferenceNode* currentNode = &rootNode;

	//Lob one level off from the path's string creating nodes along the way
	while (path.find("/") != std::string::npos)
	{
		std::string nextNodeName = path.substr(0, path.find("/"));
		path = path.substr(path.find("/")+1, std::string::npos);

		//Check and see if the next node already exists...
		bool needNewNode = true;
		for (unsigned int a = 0; a < currentNode->childNodes.size(); a++)
		{
			if (currentNode->childNodes[a].name == nextNodeName)
			{
				currentNode = &currentNode->childNodes[a];
				needNewNode = false;
				break;
			}
		}

		//... it does not, create it
		if(needNewNode)
		{
			currentNode->childNodes.emplace_back();
			currentNode->childNodes.back().parent = currentNode;
			currentNode->childNodes.back().name = nextNodeName;
			currentNode = &currentNode->childNodes.back();
		}
	}

	//Return a node so the add* functions can easily add their preference
	return currentNode;
}

void SettingManager::addInt(std::string path, int value,bool overwrite,std::string desc)
{
	PreferenceNode *target = createPathTo(path);

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a].name == path)
		{
			if(overwrite)
				target->childPreferences[a].value = std::to_string(value);
			target->description = desc;
			return;
		}
	}

	target->childPreferences.emplace_back();
	target->childPreferences.back().value = std::to_string(value);
	target->childPreferences.back().type = PreferenceInteger;
	target->childPreferences.back().name = path;
	target->description = desc;
}

void SettingManager::addString(std::string path, std::string value, bool overwrite,std::string desc)
{
	PreferenceNode* target = createPathTo(path);

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a].name == path)
		{
			if(overwrite)
				target->childPreferences[a].value = value;
			target->description = desc;
			return;
		}
	}

	target->childPreferences.emplace_back();
	target->childPreferences.back().value = value;
	target->childPreferences.back().type = PreferenceString;
	target->childPreferences.back().name = path;
	target->description = desc;
}

void SettingManager::addBool(std::string path, bool value, bool overwrite,std::string desc)
{
	PreferenceNode* target = createPathTo(path);

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a].name == path)
		{
			if(overwrite)
				target->childPreferences[a].value = value ? "true" : "false";
			target->description = desc;
			return;
		}
	}

	target->childPreferences.emplace_back();
	target->childPreferences.back().value = value ? "true" : "false";
	target->childPreferences.back().type = PreferenceBoolean;
	target->childPreferences.back().name = path;
	target->description = desc;
}

void SettingManager::addFloat(std::string path, float value, bool overwrite,std::string desc)
{
	PreferenceNode* target = createPathTo(path);

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a].name == path)
		{
			if(overwrite)
				target->childPreferences[a].value = std::to_string(value);
			target->description = desc;
			return;
		}
	}

	target->childPreferences.emplace_back();
	target->childPreferences.back().value = std::to_string(value);
	target->childPreferences.back().type = PreferenceFloat;
	target->childPreferences.back().name = path;
	target->description = desc;
}

float SettingManager::getFloat(std::string path) const
{
	PreferencePair const * const pair = getPreference(path);
	if (pair)
		return (float)atof(pair->value.c_str()); //Supress warning, we know it's single precision
	else
		return 0;
}

int SettingManager::getInt(std::string path) const
{
	PreferencePair const* const pair = getPreference(path);
	if (pair)
		return atoi(pair->value.c_str());
	else
		return 0;
}

std::string SettingManager::getString(std::string path) const
{
	PreferencePair const* const pair = getPreference(path);
	if (pair)
		return pair->value;
	else
		return "";
}

bool SettingManager::getBool(std::string path) const
{
	PreferencePair const* const pair = getPreference(path);
	if (pair)
		return pair->value == "true";
	else
		return false;
}

std::string SettingManager::operator[] (std::string path) const
{
	PreferencePair const* const pref = getPreference(path);
	if (pref)
		return pref->value;
	else
		return "";
}

PreferencePair const* const SettingManager::getPreference(std::string path) const
{
	//Saves headaches down the road :)
	path = lowercase(path);

	PreferenceNode const * currentNode = &rootNode;

	while (path.find("/") != std::string::npos)
	{
		std::string nextNodeName = path.substr(0, path.find("/"));
		path = path.substr(path.find("/") + 1, std::string::npos);
		 
		bool foundChildNode = false;

		for (unsigned int a = 0; a < currentNode->childNodes.size(); a++)
		{
			if (currentNode->childNodes[a].name == nextNodeName)
			{
				currentNode = &currentNode->childNodes[a];
				foundChildNode = true;
				break;
			}
		}

		//This part of the path is not a child node
		//Either the path is wrong, or we're at our leaf node i.e. preference
		if (!foundChildNode)
			break;
	}

	//Couldn't find a node yet we're clearly not at the end of the path
	//Must be a bad path
	if (path.find("/") != std::string::npos)
		return nullptr;

	for (unsigned int a = 0; a < currentNode->childPreferences.size(); a++)
	{
		if (currentNode->childPreferences[a].name == path)
		{
			return &currentNode->childPreferences[a];
		}
	}

	//Found up to the last node, but the last part of the path was invalid
	return nullptr;
}

void PreferenceNode::readFromLine(std::string &line,int lineNumber,PreferenceNode *currentNode)
{
	//Preference lines look like this:
	//[initial tabs]Preference Name[one tab]Preference Type[one tab]Value[new line]

	//Remove initial tabs
	line = leftTrim(line);

	//Make sure we actually have a name type and value separated by tabs
	size_t tabAfterName = line.find("\t");
	if (tabAfterName == std::string::npos)
	{
		error("Missing name parameter on line " + std::to_string(lineNumber));
		return;
	}

	size_t tabAfterType = line.find("\t", tabAfterName + 1);
	if (tabAfterType == std::string::npos)
	{
		error("Missing type parameter on line " + std::to_string(lineNumber));
		return;
	}

	if (line.length() - tabAfterType < 1)
	{
		error("Missing value for parameter on line " + std::to_string(lineNumber));
		return;
	}

	//Convert to lowercase
	std::string name = lowercase(line.substr(0, tabAfterName));
	std::string type = lowercase(line.substr(tabAfterName + 1, tabAfterType - (tabAfterName + 1)));
	std::string value = line.substr(tabAfterType + 1, line.length() - (tabAfterType + 1));

	//Add the preference
	currentNode->childPreferences.emplace_back();
	PreferencePair* pair = &currentNode->childPreferences.back();

	//Make sure type makes sense
	if (type == "boolean")
		pair->type = PreferenceBoolean;
	else if (type == "float")
		pair->type = PreferenceFloat;
	else if (type == "string")
		pair->type = PreferenceString;
	else if (type == "integer")
		pair->type = PreferenceInteger;
	else
	{
		error("Invalid type " + type + " of line " + std::to_string(lineNumber));
		return;
	}

	pair->name = name;
	pair->value = value;
}

SettingManager::SettingManager(std::string path)
{
	scope("SettingManager::SettingManager");
	debug("Parsing " + path);

	//Make sure the file we're reading from is valid
	std::ifstream prefsFile(path);
	if (!prefsFile.is_open())
	{
		error("Could not open preferences file " + path);
		return;
	}

	//Construct a tree by reading a text file
	int lastNumberOfTabs = 0;
	PreferenceNode* currentNode = &rootNode;

	std::string line = "";
	int lineNumber = 0;
	int readPrefs = 0;
	while (!prefsFile.eof())
	{
		lineNumber++;
		getline(prefsFile, line);

		//Ignore blank lines
		if (line.length() < 1)
			continue;

		//Allow comment lines if they start with #
		if (line[0] == '#')
			continue;

		//How many tabs are at the start of this line
		//Tabs determine how far down the tree we are
		int currentNumberOfTabs = 0;
		for (; line[currentNumberOfTabs] == '\t'; currentNumberOfTabs++);

		if (currentNumberOfTabs > lastNumberOfTabs + 1)
		{
			error("Skipped a level of indentation at line " + std::to_string(lineNumber));
			return;
		}

		//We indented once, go up the tree one node
		if (currentNumberOfTabs > lastNumberOfTabs)
		{
			if (currentNode->childNodes.size() < 1)
			{
				error("Indentation - attempt to add to child node failed because no child node existed on line " + std::to_string(lineNumber));
				return;
			}

			//Find whatever the last added node is and set that as the current node, before doing whatever
			currentNode = &currentNode->childNodes.back();
		}
		//We went back a few levels of identation, go back down the tree
		else if (currentNumberOfTabs < lastNumberOfTabs)
		{
			int toGoBack = lastNumberOfTabs - currentNumberOfTabs;

			for (int i = 0; i < toGoBack; i++)
			{
				if (!currentNode->parent)
				{
					error("Somehow attempted to go back beyond root node on line " + std::to_string(lineNumber));
					return;
				}

				currentNode = currentNode->parent;
			}
		}

		//No extra fields, just adding a new node
		if (line.find("\t", currentNumberOfTabs) == std::string::npos)
		{
			//The rest of the line after the tabs will be the new node's name
			std::string nodeName = line.substr(currentNumberOfTabs, std::string::npos);
			if (nodeName.length() < 1)
			{
				error("Unnamed node on line " + std::to_string(lineNumber));
				return;
			}

			currentNode->childNodes.emplace_back();
			currentNode->childNodes.back().parent = currentNode;
			currentNode->childNodes.back().name = lowercase(nodeName);
		}
		//Extra fields, must be an actual setting
		else
		{
			readPrefs++;
			currentNode->readFromLine(line, lineNumber, currentNode);
		}

		lastNumberOfTabs = currentNumberOfTabs;
	}

	debug("Read " + std::to_string(readPrefs) + " prefs from " + std::to_string(lineNumber) + " lines");
}   

void SettingManager::writePreferenceWarningToFile(std::ofstream& file)
{
	file << "#\tThis is a comment line, any line with a # will be ignored.\n";
	file << "#\tBy default comments will be written over next time settings are saved.\n";
	file << "#\tIt is not recomended to edit these settings files manually.\n";
	file << "#\tThe formatting and whitespace is very specific and should not be changed\n";
}

void PreferenceNode::writeToFile(std::ofstream& file,int level) const
{
	//We don't need to write the rootNode to file as it has no name
	if (parent)
	{
		//This line can create a new preference node upon loading
		//We don't need to indent after the rootNode
		for (int i = 0; i < level-1; i++)
			file << "\t";
		file << name << "\n";
	}

	//Then write the preferences
	for (unsigned int i = 0; i < childPreferences.size(); i++)
	{
		//All of this is one line
		for (int i = 0; i < level; i++)
			file << "\t";
		file << childPreferences[i].name << "\t";
		switch (childPreferences[i].type)
		{
			case PreferenceBoolean:
				file << "boolean\t";
				break;
			case PreferenceFloat:
				file << "float\t";
				break;
			case PreferenceInteger:
				file << "integer\t";
				break;
			case PreferenceString:
				file << "string\t";
				break;
		}
		file << childPreferences[i].value << "\n";
	}

	//Recursivly write child nodes:
	for (unsigned int i = 0; i < childNodes.size(); i++)
		childNodes[i].writeToFile(file, level+1);
}

void SettingManager::exportToFile(std::string path) const
{
	std::filesystem::create_directories(std::filesystem::path(path.c_str()).parent_path());

	std::ofstream file(path.c_str());

	if (!file.is_open())
	{
		error("Could not open file " + path + " for exporting preferences.");
		return;
	}

	writePreferenceWarningToFile(file);

	//Recursive!
	rootNode.writeToFile(file);

	file.close();
} 
