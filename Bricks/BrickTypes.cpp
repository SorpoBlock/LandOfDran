#include "BrickTypes.h"

static void stripCarriageReturn(std::string& line)
{
	if (!line.empty() && line.back() == '\r')
		line.pop_back();
}

static bool parseBlocklandBasicName(std::string name, int& width, int& height, int& length)
{
	bool plate = !name.empty() && name.back() == 'f';
	if (plate)
		name.pop_back();

	std::vector<int> numbers;
	size_t start = 0;
	while (true)
	{
		size_t separator = name.find('x', start);
		std::string part = name.substr(start, separator == std::string::npos ? std::string::npos : separator - start);

		if (part.empty() || part.length() > 3 || part.find_first_not_of("0123456789") != std::string::npos)
			return false;

		numbers.push_back(std::stoi(part));

		if (separator == std::string::npos)
			break;
		start = separator + 1;
	}

	if (numbers.size() == 2)
	{
		width = numbers[0];
		length = numbers[1];
		height = plate ? 1 : 3;
	}
	else if (numbers.size() == 3 && !plate)
	{
		width = numbers[0];
		length = numbers[1];
		height = numbers[2] * 3;
	}
	else
		return false;

	return width >= 1 && width <= 255 && length >= 1 && length <= 255 && height >= 1 && height <= 255;
}

void BrickTypes::load(const std::string& typesFolder)
{
	scope("BrickTypes::load");

	basicTypes.clear();
	basicByName.clear();
	specialNames.clear();

	std::error_code errorCode;
	if (!std::filesystem::is_directory(typesFolder, errorCode))
	{
		error("Brick types folder " + typesFolder + " not found");
		return;
	}

	//Lowercase .blb file name to its path
	std::unordered_map<std::string, std::filesystem::path> blbFiles;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(typesFolder, errorCode))
	{
		if (entry.is_regular_file() && lowercase(entry.path().extension().string()) == ".blb")
			blbFiles[lowercase(entry.path().filename().string())] = entry.path();
	}

	std::ifstream aliases(typesFolder + "/test.cs", std::ios::binary);
	if (!aliases.is_open())
	{
		error("Could not open " + typesFolder + "/test.cs");
		return;
	}

	//Type name to .blb path, from test.cs
	std::vector<std::pair<std::string, std::filesystem::path>> namedFiles;
	std::unordered_set<std::string> claimedFiles;

	std::string line;
	while (getline(aliases, line))
	{
		stripCarriageReturn(line);

		size_t bar = line.find('|');
		if (bar == std::string::npos)
			continue;

		std::string fileName = lowercase(line.substr(bar + 1));
		auto blb = blbFiles.find(fileName);

		//Most names in the list have no .blb in the starter pack, getBasicSize falls back to parsing those names
		if (blb == blbFiles.end())
			continue;

		namedFiles.push_back({ line.substr(0, bar), blb->second });
		claimedFiles.insert(fileName);
	}

	//Some .blb files are named after their type instead of being listed in test.cs, e.g. "16x32 Base.blb"
	for (const auto& entry : blbFiles)
	{
		if (!claimedFiles.count(entry.first))
			namedFiles.push_back({ entry.second.stem().string(), entry.second });
	}

	for (const auto& [name, path] : namedFiles)
	{
		auto blb = blbFiles.find(lowercase(path.filename().string()));

		std::ifstream blbFile(blb->second, std::ios::binary);
		std::string sizeLine, kindLine;
		getline(blbFile, sizeLine);
		getline(blbFile, kindLine);
		stripCarriageReturn(kindLine);

		if (kindLine == "SPECIAL" || kindLine == "SPECIALBRICK")
		{
			specialNames.insert(lowercase(name));
			continue;
		}

		int width = 0, length = 0, height = 0;
		if (kindLine != "BRICK" || sscanf(sizeLine.c_str(), "%d %d %d", &width, &length, &height) != 3 ||
			width < 1 || width > 255 || length < 1 || length > 255 || height < 1 || height > 255)
		{
			error("Invalid brick type file " + blb->second.string());
			continue;
		}

		BasicBrickType type;
		type.uiName = name;
		type.width = width;
		type.height = height;
		type.length = length;

		std::filesystem::path icon = blb->second;
		icon.replace_extension(".png");
		if (std::filesystem::exists(icon, errorCode))
			type.iconPath = icon.string();

		basicByName[lowercase(name)] = basicTypes.size();
		basicTypes.push_back(type);
	}

	info("Loaded " + std::to_string(basicTypes.size()) + " basic and " + std::to_string(specialNames.size()) + " special brick types");
}

bool BrickTypes::getBasicSize(const std::string& name, int& width, int& height, int& length) const
{
	std::string lowerName = lowercase(name);

	auto found = basicByName.find(lowerName);
	if (found != basicByName.end())
	{
		const BasicBrickType& type = basicTypes[found->second];
		width = type.width;
		height = type.height;
		length = type.length;
		return true;
	}

	if (specialNames.count(lowerName))
		return false;

	return parseBlocklandBasicName(lowerName, width, height, length);
}
