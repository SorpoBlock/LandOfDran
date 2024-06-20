#include "SettingManager.h"

/*
	Used in DrawSettingsWindow in a loop to get names, types, and pointers for each preference
	Returns true if there's still preferences, and false when the loop should end
	prePath is the level of the preference hierarchy you want to parse at, which may be left blank
*/
PreferencePair *SettingManager::nextPreferenceBinding(std::string &path)
{
	//No nodes, settings not loaded yet, or at the end of the search
	if (nodeSearchIndex >= allNodes.size())
		return nullptr;

	//Done with that particular node...
	if (preferenceSearchIndex >= allNodes[nodeSearchIndex]->childPreferences.size())
	{
		nodeSearchIndex++;
		preferenceSearchIndex = 0;
	}

	//..turns out that particular node was the last node
	if (nodeSearchIndex >= allNodes.size())
		return nullptr;

	PreferencePair* ret = allNodes[nodeSearchIndex]->childPreferences[preferenceSearchIndex];
	path = allNodes[nodeSearchIndex]->path;
	preferenceSearchIndex++;
	return ret;
}

void SettingManager::startPreferenceBindingSearch()
{
	preferenceSearchIndex = 0;
	nodeSearchIndex = 0;
}

void SettingManager::addEnum(std::string path, int value,std::string desc,std::vector<std::string> &&names)
{
	std::string pathBeforePref = path;
	PreferenceNode *target = createPathTo(path);

	//Get the path of the nodes before the preference name
	if (path.length() + 1 < pathBeforePref.length())
		pathBeforePref = pathBeforePref.substr(0, pathBeforePref.length() - (path.length() + 1));
	target->path = pathBeforePref;

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a]->name == path)
		{
			target->childPreferences[a]->description = desc;
			target->childPreferences[a]->minValue = 0;
			target->childPreferences[a]->maxValue = names.size();
			target->childPreferences[a]->dropDownNames = std::move(names);
			return;
		}
	}

	PreferencePair* tmp = new PreferencePair;
	target->childPreferences.push_back(tmp);
	tmp->value = std::to_string(value);
	tmp->valueInt = value;
	tmp->type = PreferenceInteger;
	tmp->name = path;
	tmp->description = desc;
	tmp->minValue = 0;
	tmp->maxValue = names.size();
	tmp->dropDownNames = std::move(names);
}

PreferenceNode* SettingManager::createPathTo(std::string &path)
{
	//Why would this ever happen?
	if (path.length() == 0)
		return &rootNode;

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
			if (currentNode->childNodes[a]->name == nextNodeName)
			{
				currentNode = currentNode->childNodes[a];
				needNewNode = false;
				break;
			}
		}

		//... it does not, create it
		if(needNewNode)
		{
			PreferenceNode* tmp = new PreferenceNode;
			currentNode->childNodes.push_back(tmp);
			tmp->parent = currentNode;
			tmp->name = nextNodeName;
			currentNode = currentNode->childNodes.back();
			currentNode->path = path;
			allNodes.push_back(currentNode);
		}
	}

	//Return a node so the add* functions can easily add their preference
	return currentNode;
}

void SettingManager::addInt(std::string path, int value,bool overwrite,std::string desc, float min, float max)
{
	std::string pathBeforePref = path;
	PreferenceNode* target = createPathTo(path);

	//Get the path of the nodes before the preference name
	if (path.length() + 1 < pathBeforePref.length())
		pathBeforePref = pathBeforePref.substr(0, pathBeforePref.length() - (path.length() + 1));
	target->path = pathBeforePref;

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a]->name == path)
		{
			if (overwrite)
			{
				target->childPreferences[a]->value = std::to_string(value);
				target->childPreferences[a]->valueInt = value;
			}
			target->childPreferences[a]->description = desc;
			target->childPreferences[a]->minValue = min;
			target->childPreferences[a]->maxValue = max;
			return;
		}
	}

	PreferencePair* tmp = new PreferencePair;
	target->childPreferences.push_back(tmp);
	tmp->value = std::to_string(value);
	tmp->valueInt = value;
	tmp->type = PreferenceInteger;
	tmp->name = path;
	tmp->description = desc;
	tmp->minValue = min;
	tmp->maxValue = max;
}

void SettingManager::addString(std::string path, std::string value, bool overwrite,std::string desc)
{
	std::string pathBeforePref = path;
	PreferenceNode* target = createPathTo(path);

	//Get the path of the nodes before the preference name
	if (path.length() + 1 < pathBeforePref.length())
		pathBeforePref = pathBeforePref.substr(0, pathBeforePref.length() - (path.length() + 1));
	target->path = pathBeforePref;

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a]->name == path)
		{
			if (overwrite)
				target->childPreferences[a]->value = value;
			target->childPreferences[a]->description = desc;
			return;
		}
	}

	PreferencePair* tmp = new PreferencePair;
	target->childPreferences.push_back(tmp);
	tmp->value = value;
	tmp->type = PreferenceString;
	tmp->name = path;
	tmp->description = desc;
}

void SettingManager::addBool(std::string path, bool value, bool overwrite,std::string desc)
{
	std::string pathBeforePref = path;
	PreferenceNode* target = createPathTo(path);

	//Get the path of the nodes before the preference name
	if (path.length() + 1 < pathBeforePref.length())
		pathBeforePref = pathBeforePref.substr(0, pathBeforePref.length() - (path.length() + 1));
	target->path = pathBeforePref;

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a]->name == path)
		{
			if (overwrite)
			{
				target->childPreferences[a]->value = value ? "true" : "false";
				target->childPreferences[a]->valueBool = value;
			}
			target->childPreferences[a]->description = desc;
			return;
		}
	}

	PreferencePair* tmp = new PreferencePair;
	target->childPreferences.push_back(tmp);
	tmp->value = value ? "true" : "false";
	tmp->valueBool = value;
	tmp->type = PreferenceBoolean;
	tmp->name = path;
	tmp->description = desc;
}

void SettingManager::addColor(std::string path, glm::vec4 value, bool overwrite, std::string desc)
{
	std::string pathBeforePref = path;
	PreferenceNode* target = createPathTo(path);

	//Get the path of the nodes before the preference name
	if (path.length() + 1 < pathBeforePref.length())
		pathBeforePref = pathBeforePref.substr(0, pathBeforePref.length() - (path.length() + 1));
	target->path = pathBeforePref;

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a]->name == path)
		{
			if (overwrite)
			{
				target->childPreferences[a]->value = std::to_string(value.r) + " " + std::to_string(value.g) + " " + std::to_string(value.b) + " " + std::to_string(value.a);
				target->childPreferences[a]->color[0] = value.r;
				target->childPreferences[a]->color[1] = value.g;
				target->childPreferences[a]->color[2] = value.b;
				target->childPreferences[a]->color[3] = value.a;
			}
			target->childPreferences[a]->description = desc;
			target->childPreferences[a]->type = PreferenceColor;
			return;
		}
	}

	PreferencePair* tmp = new PreferencePair;
	target->childPreferences.push_back(tmp);
	tmp->value = std::to_string(value.r) + " " + std::to_string(value.g) + " " + std::to_string(value.b) + " " + std::to_string(value.a);
	tmp->color[0] = value.r;
	tmp->color[1] = value.g;
	tmp->color[2] = value.b;
	tmp->color[3] = value.a;
	tmp->type = PreferenceColor;
	tmp->name = path;
	tmp->description = desc;
}

void SettingManager::addFloat(std::string path, float value, bool overwrite,std::string desc, float min, float max)
{
	std::string pathBeforePref = path;
	PreferenceNode* target = createPathTo(path);

	//Get the path of the nodes before the preference name
	if (path.length() + 1 < pathBeforePref.length())
		pathBeforePref = pathBeforePref.substr(0, pathBeforePref.length() - (path.length() + 1));
	target->path = pathBeforePref;

	for (unsigned int a = 0; a < target->childPreferences.size(); a++)
	{
		if (target->childPreferences[a]->name == path)
		{
			if (overwrite)
			{
				target->childPreferences[a]->value = std::to_string(value);
				target->childPreferences[a]->valueFloat = value;
			}
			target->childPreferences[a]->description = desc;
			target->childPreferences[a]->minValue = min;
			target->childPreferences[a]->maxValue = max;
			return;
		}
	}

	PreferencePair* tmp = new PreferencePair;
	target->childPreferences.push_back(tmp);
	tmp->value = std::to_string(value);
	tmp->type = PreferenceFloat;
	tmp->valueFloat = value;
	tmp->name = path;
	tmp->description = desc;
	tmp->minValue = min;
	tmp->maxValue = max;
}

float SettingManager::getFloat(std::string path) const
{
	PreferencePair const * const pair = getPreference(path);
	if (pair)
		return (float)atof(pair->value.c_str()); //Supress warning, we know it's single precision
	else
		return 0;
}

glm::vec4 SettingManager::getColor(std::string path) const
{
	glm::vec4 ret(1, 1, 1, 1);
	PreferencePair const* const pair = getPreference(path);
	if (pair)
	{
		//Break string like '1 0.5 0.2 0.9' into four floats for color
		std::string buf = "";
		std::stringstream ss(pair->value);
		ss >> buf;
		if (buf.length() < 1)
			return ret;
		ret.r = atof(buf.c_str());
		ss >> buf;
		if (buf.length() < 1)
			return ret;
		ret.g = atof(buf.c_str());
		ss >> buf;
		if (buf.length() < 1)
			return ret;
		ret.b = atof(buf.c_str());
		ss >> buf;
		if (buf.length() < 1)
			return ret;
		ret.a = atof(buf.c_str());
		return ret;
	}
	else
		return ret;
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

PreferencePair /*const* const*/* SettingManager::getPreference(std::string path) const
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
			if (currentNode->childNodes[a]->name == nextNodeName)
			{
				currentNode = currentNode->childNodes[a];
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
		if (currentNode->childPreferences[a]->name == path)
		{
			return currentNode->childPreferences[a];
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

	size_t tabAfterValue = line.find("\t", tabAfterType + 1);
	if (tabAfterValue == std::string::npos)
	{
		error("Missing value parameter on line " + std::to_string(lineNumber));
		return;
	}

	if (line.length() - tabAfterValue < 1)
	{
		error("Missing value for metadata on line " + std::to_string(lineNumber));
		return;
	}

	//Convert to lowercase
	std::string name = lowercase(line.substr(0, tabAfterName));
	std::string type = lowercase(line.substr(tabAfterName + 1, tabAfterType - (tabAfterName + 1)));
	std::string value = lowercase(line.substr(tabAfterType + 1, tabAfterValue - (tabAfterType + 1)));
	std::string meta = line.substr(tabAfterValue + 1, line.length() - (tabAfterValue + 1));

	//Add the preference
	PreferencePair* pair = new PreferencePair;
	currentNode->childPreferences.push_back(pair);

	//Make sure type makes sense, handle metadata as well, values are handled below
	if (type == "boolean")
	{
		pair->type = PreferenceBoolean;
		pair->description = meta;
	}
	else if (type == "vec4" || type == "vec")
	{
		pair->type = PreferenceColor;
		pair->description = meta;
	}
	else if (type == "float")
	{
		pair->type = PreferenceFloat;

		size_t tabAfterDesc = meta.find("\t");
		if (tabAfterDesc == std::string::npos)
		{
			error("Missing min value metadata on line " + std::to_string(lineNumber));
			return;
		}

		size_t tabAfterMin = meta.find("\t", tabAfterDesc + 1);
		if (tabAfterValue == std::string::npos)
		{
			error("Missing max value metadata on line " + std::to_string(lineNumber));
			return;
		}

		pair->description = meta.substr(0, tabAfterDesc);
		pair->minValue = atof(meta.substr(tabAfterDesc + 1, tabAfterMin - (tabAfterDesc + 1)).c_str());
		pair->maxValue = atof(meta.substr(tabAfterMin + 1, meta.length() - (tabAfterMin + 1)).c_str());
	}
	else if (type == "string")
	{
		pair->type = PreferenceString;
		pair->description = meta;
	}
	else if (type == "integer")
	{
		pair->type = PreferenceInteger;

		size_t tabAfterDesc = meta.find("\t");
		if (tabAfterDesc == std::string::npos)
		{
			error("Missing min value metadata on line " + std::to_string(lineNumber));
			return;
		}

		size_t tabAfterMin = meta.find("\t", tabAfterDesc + 1);
		if (tabAfterValue == std::string::npos)
		{
			error("Missing max value metadata on line " + std::to_string(lineNumber));
			return;
		}

		pair->description = meta.substr(0, tabAfterDesc);
		pair->minValue = atof(meta.substr(tabAfterDesc + 1, tabAfterMin - (tabAfterDesc + 1)).c_str());
		pair->maxValue = atof(meta.substr(tabAfterMin + 1, meta.length() - (tabAfterMin + 1)).c_str());
	}
	else
	{
		error("Invalid type " + type + " of line " + std::to_string(lineNumber));
		return;
	}

	pair->name = name;

	//Handle values
	pair->value = value;
	if (pair->type == PreferenceBoolean)
		pair->valueBool = value == "true";
	else if (pair->type == PreferenceFloat)
		pair->valueFloat = atof(value.c_str());
	else if (pair->type == PreferenceInteger)
		pair->valueInt = atoi(value.c_str());
	else if (pair->type == PreferenceColor)
	{
		//Break string like '1 0.5 0.2 0.9' into four floats for color
		std::string buf = "";
		std::stringstream ss(value);
		ss >> buf;
		if (buf.length() < 1)
			return;
		pair->color[0] = atof(buf.c_str());
		ss >> buf;
		if (buf.length() < 1)
			return;
		pair->color[1] = atof(buf.c_str());
		ss >> buf;
		if (buf.length() < 1)
			return;
		pair->color[2] = atof(buf.c_str());
		ss >> buf;
		if (buf.length() < 1)
			return;
		pair->color[3] = atof(buf.c_str());
	}
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

	std::vector<std::string> pathStack;

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
			currentNode = currentNode->childNodes.back();
			pathStack.push_back(currentNode->name);
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
				pathStack.pop_back();
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

			PreferenceNode* tmp = new PreferenceNode;
			allNodes.push_back(tmp);
			currentNode->childNodes.push_back(tmp);

			tmp->parent = currentNode;
			tmp->name = lowercase(nodeName);
			for (unsigned int a = 0; a < pathStack.size(); a++)
				tmp->path += pathStack[a] + "/";
			tmp->path += tmp->name;
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
		file << childPreferences[i]->name << "\t";
		switch (childPreferences[i]->type)
		{
			case PreferenceColor:
				file << "vec4\t";
				break;
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
		file << childPreferences[i]->value<<"\t";
		//Why tf did I set up my file this way lol
		switch (childPreferences[i]->type)
		{
			case PreferenceColor:
				file << childPreferences[i]->description;
				break;
			case PreferenceBoolean:
				file << childPreferences[i]->description;
				break;
			case PreferenceFloat:
				file << childPreferences[i]->description << "\t";
				file << childPreferences[i]->minValue << "\t";
				file << childPreferences[i]->maxValue;
				break;
			case PreferenceInteger:
				file << childPreferences[i]->description<<"\t";
				file << childPreferences[i]->minValue<<"\t";
				file << childPreferences[i]->maxValue;
				break;
			case PreferenceString:
				file << childPreferences[i]->description;
				break;
		}
		file << "\n";
	}

	//Recursivly write child nodes:
	for (unsigned int i = 0; i < childNodes.size(); i++)
		childNodes[i]->writeToFile(file, level+1);
}

void SettingManager::exportToFile(std::string path) const
{
	//This loop exists in case someone changed the options in game
	for (unsigned int a = 0; a < allNodes.size(); a++)
	{
		for (unsigned int b = 0; b < allNodes[a]->childPreferences.size(); b++)
		{
			PreferencePair *pref = allNodes[a]->childPreferences[b];
			if (pref->type == PreferenceBoolean)
				pref->value = pref->valueBool ? "true" : "false";
			if (pref->type == PreferenceInteger)
				pref->value = std::to_string(pref->valueInt);
			if (pref->type == PreferenceFloat)
				pref->value = std::to_string(pref->valueFloat);
			if (pref->type == PreferenceColor)
				pref->value = std::to_string(pref->color[0]) + " " + std::to_string(pref->color[1]) + " " + std::to_string(pref->color[2]) + " " + std::to_string(pref->color[3]);
		}
	}

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
