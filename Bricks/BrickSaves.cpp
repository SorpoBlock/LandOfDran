#include "BrickSaves.h"

//The "next version" (16483535) adds owner, name, and flag fields to every brick record
static constexpr unsigned int lodMagic = 16483534;

//Fixed size parts of old save brick records
static constexpr std::streamoff basicRecordBytes = 4 + 3 * 4 + 4 + 2;
static constexpr std::streamoff specialRecordBytes = 4 + 3 * 4 + 4 + 2;

std::string getSavePath(const std::string& fileName)
{
	if (fileName.empty() || fileName.length() > 128 || fileName[0] == '.')
		return "";

	if (fileName.find_first_of("/\\:") != std::string::npos || fileName.find("..") != std::string::npos)
		return "";

	return "Saves/" + fileName;
}

static void writeValue(std::ofstream& file, const auto& value)
{
	file.write((const char*)&value, sizeof(value));
}

static bool readValue(std::ifstream& file, auto& value)
{
	return (bool)file.read((char*)&value, sizeof(value));
}

bool saveLodBuild(const BrickHolder& bricks, const std::string& path, bool omitOwnership)
{
	scope("saveLodBuild");

	std::error_code errorCode;
	std::filesystem::create_directories(std::filesystem::path(path).parent_path(), errorCode);

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		error("Could not open " + path + " for writing");
		return false;
	}

	unsigned int count = bricks.size();
	writeValue(file, lodMagic + 1);
	writeValue(file, count);
	writeValue(file, (unsigned int)0); //Special brick type names

	writeValue(file, count);
	for (size_t a = 0; a < bricks.size(); a++)
	{
		const Brick* brick = bricks.get(a);

		for (int channel = 0; channel < 4; channel++)
			writeValue(file, (unsigned char)brick->color[channel]);

		//Brick centers, x and z in studs, y in world units
		writeValue(file, (float)(brick->x + brick->footprintWidth() * 0.5));
		writeValue(file, (float)((brick->y + brick->height * 0.5) * PLATE_SIZE));
		writeValue(file, (float)(brick->z + brick->footprintLength() * 0.5));

		writeValue(file, brick->width);
		writeValue(file, brick->height);
		writeValue(file, brick->length);
		writeValue(file, (unsigned char)0); //Print mask
		writeValue(file, brick->angleID);
		writeValue(file, (unsigned char)0); //Material

		writeValue(file, omitOwnership ? -1 : brick->ownerID);

		std::string name = brick->name.substr(0, 255);
		writeValue(file, (unsigned char)name.length());
		file.write(name.c_str(), name.length());

		writeValue(file, (unsigned char)(brick->collides ? 1 : 0));
	}

	writeValue(file, (unsigned int)0); //Transparent basic bricks, the old game only filled this in when clients saved
	writeValue(file, (unsigned int)0); //Special bricks
	writeValue(file, (unsigned int)0);

	if (!file)
	{
		error("Error while writing " + path);
		return false;
	}

	info("Saved " + std::to_string(count) + " bricks to " + path);
	return true;
}

/*
	Skips the owner, name, flags, and optional music/light/print data at the end of a next version record
	Returns false if the file ran out
*/
static bool readRecordExtras(std::ifstream& file, bool& collides, std::string& name)
{
	int ownerID;
	unsigned char nameLength, flags;
	if (!readValue(file, ownerID) || !readValue(file, nameLength))
		return false;

	name.resize(nameLength);
	if (nameLength > 0 && !file.read(&name[0], nameLength))
		return false;

	if (!readValue(file, flags))
		return false;

	collides = flags & 1;

	//Music: track and pitch
	if (flags & 2)
		file.seekg(sizeof(unsigned int) + sizeof(float), std::ios::cur);

	//Light: rgb
	if (flags & 4)
		file.seekg(3 * sizeof(float), std::ios::cur);

	//Print: mask then name
	if (flags & 8)
	{
		unsigned char mask, printNameLength;
		if (!readValue(file, mask) || !readValue(file, printNameLength))
			return false;
		file.seekg(printNameLength, std::ios::cur);
	}

	return (bool)file;
}

int loadLodBuild(BrickHolder& bricks, const std::string& path, int offsetX, int offsetY, int offsetZ)
{
	scope("loadLodBuild");

	unsigned int startMS = SDL_GetTicks();

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		error("Could not open " + path);
		return -1;
	}

	unsigned int magic, brickCount, typeCount;
	if (!readValue(file, magic) || (magic != lodMagic && magic != lodMagic + 1))
	{
		error(path + " is not a Land of Dran binary save");
		return -1;
	}
	bool hasExtras = magic == lodMagic + 1;

	if (!readValue(file, brickCount) || !readValue(file, typeCount))
	{
		error(path + " ended early");
		return -1;
	}

	for (unsigned int a = 0; a < typeCount; a++)
	{
		unsigned char nameLength;
		if (!readValue(file, nameLength))
		{
			error(path + " ended early");
			return -1;
		}
		file.seekg(nameLength, std::ios::cur);
	}

	int loaded = 0;
	int rejected = 0;
	int skippedSpecial = 0;
	int invalid = 0;

	//Opaque basic, transparent basic, then special bricks
	for (int section = 0; section < 3; section++)
	{
		unsigned int sectionCount;
		if (!readValue(file, sectionCount))
		{
			error(path + " ended early");
			break;
		}

		for (unsigned int a = 0; a < sectionCount; a++)
		{
			unsigned char color[4];
			float centerX, centerY, centerZ;
			unsigned char width = 0, height = 0, length = 0, printMask, angleID, material;
			unsigned int typeID;

			bool ok = file.read((char*)color, 4) && readValue(file, centerX) && readValue(file, centerY) && readValue(file, centerZ);
			if (section == 2)
				ok = ok && readValue(file, typeID);
			else
				ok = ok && readValue(file, width) && readValue(file, height) && readValue(file, length) && readValue(file, printMask);
			ok = ok && readValue(file, angleID) && readValue(file, material);

			bool collides = true;
			std::string name = "";
			if (ok && hasExtras)
				ok = readRecordExtras(file, collides, name);

			if (!ok)
			{
				error(path + " ended early, loaded " + std::to_string(loaded) + " bricks");
				return loaded;
			}

			if (section == 2)
			{
				skippedSpecial++;
				continue;
			}

			if (width == 0 || height == 0 || length == 0 || angleID > 3)
			{
				invalid++;
				continue;
			}

			Brick desc;
			desc.width = width;
			desc.height = height;
			desc.length = length;
			desc.angleID = angleID;
			desc.color = glm::u8vec4(color[0], color[1], color[2], color[3]);
			desc.collides = collides;
			desc.name = name;
			desc.x = (int)lround(centerX - desc.footprintWidth() * 0.5) + offsetX;
			desc.y = (int)lround(centerY / PLATE_SIZE - height * 0.5) + offsetY;
			desc.z = (int)lround(centerZ - desc.footprintLength() * 0.5) + offsetZ;

			if (bricks.add(desc))
				loaded++;
			else
				rejected++;
		}
	}

	info("Loaded " + std::to_string(loaded) + " bricks from " + path + " in " + std::to_string(SDL_GetTicks() - startMS) + "ms");
	if (rejected > 0)
		info(std::to_string(rejected) + " bricks overlapped existing bricks or were out of bounds");
	if (skippedSpecial > 0)
		info(std::to_string(skippedSpecial) + " special bricks skipped, special bricks aren't supported yet");
	if (invalid > 0)
		error(std::to_string(invalid) + " bricks had invalid sizes or rotations");

	return loaded;
}

//Splits on single spaces, keeping empty fields, since an empty print name in a .bls line leaves two spaces in a row
static std::vector<std::string> splitFields(const std::string& line)
{
	std::vector<std::string> fields;
	size_t start = 0;
	while (true)
	{
		size_t space = line.find(' ', start);
		fields.push_back(line.substr(start, space == std::string::npos ? std::string::npos : space - start));
		if (space == std::string::npos)
			return fields;
		start = space + 1;
	}
}

int loadBlocklandBuild(BrickHolder& bricks, const BrickTypes& types, const std::string& path)
{
	scope("loadBlocklandBuild");

	unsigned int startMS = SDL_GetTicks();

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		error("Could not open " + path);
		return -1;
	}

	auto readLine = [&file](std::string& line) -> bool
	{
		if (!getline(file, line))
			return false;
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		return true;
	};

	std::string line;

	//Warning line, then a count of description lines followed by the description itself
	readLine(line);
	readLine(line);
	int descriptionLines = atoi(line.c_str());
	for (int a = 0; a < descriptionLines; a++)
		readLine(line);

	glm::u8vec4 palette[64];
	for (int a = 0; a < 64; a++)
	{
		float r = 1, g = 1, b = 1, alpha = 1;
		if (!readLine(line))
		{
			error(path + " ended before its color palette did");
			return -1;
		}
		sscanf(line.c_str(), "%f %f %f %f", &r, &g, &b, &alpha);
		palette[a] = glm::u8vec4(glm::clamp(glm::vec4(r, g, b, alpha), 0.0f, 1.0f) * 255.0f + 0.5f);
	}

	//"Linecount N"
	readLine(line);

	int loaded = 0;
	int rejected = 0;
	std::map<std::string, int> skippedNames;
	Brick* lastBrick = nullptr;

	while (readLine(line))
	{
		if (line.compare(0, 2, "+-") == 0)
		{
			if (line.compare(0, 15, "+-NTOBJECTNAME ") == 0 && lastBrick)
				lastBrick->name = line.substr(15);
			continue;
		}

		size_t quote = line.find('"');
		if (quote == std::string::npos || quote + 2 > line.length())
			continue;

		lastBrick = nullptr;

		std::string name = line.substr(0, quote);
		std::vector<std::string> fields = splitFields(line.substr(quote + 2));

		//x y z angle isBaseplate colorIndex print colorFx shapeFx raycasting collision rendering
		if (fields.size() < 11)
			continue;

		int width, height, length;
		if (!types.getBasicSize(name, width, height, length))
		{
			skippedNames[name]++;
			continue;
		}

		Brick desc;
		desc.width = width;
		desc.height = height;
		desc.length = length;
		desc.angleID = atoi(fields[3].c_str()) % 4;
		desc.color = palette[std::clamp(atoi(fields[5].c_str()), 0, 63)];
		desc.collides = fields[10] != "0";

		//Blockland is z-up with half-stud and fifth-of-a-world-unit units, the old game swapped y and z and doubled
		double centerX = atof(fields[0].c_str()) * 2.0;
		double centerZ = atof(fields[1].c_str()) * 2.0;
		double centerY = atof(fields[2].c_str()) * 2.0;

		desc.x = (int)lround(centerX - desc.footprintWidth() * 0.5);
		desc.y = (int)lround(centerY / PLATE_SIZE - height * 0.5);
		desc.z = (int)lround(centerZ - desc.footprintLength() * 0.5);

		lastBrick = bricks.add(desc);
		if (lastBrick)
			loaded++;
		else
			rejected++;
	}

	info("Loaded " + std::to_string(loaded) + " bricks from " + path + " in " + std::to_string(SDL_GetTicks() - startMS) + "ms");
	if (rejected > 0)
		info(std::to_string(rejected) + " bricks overlapped existing bricks or were out of bounds");

	if (!skippedNames.empty())
	{
		int skipped = 0;
		std::string list = "";
		for (const auto& entry : skippedNames)
		{
			skipped += entry.second;
			list += (list.empty() ? "" : ", ") + entry.first + " (" + std::to_string(entry.second) + ")";
		}
		info(std::to_string(skipped) + " special or unknown bricks skipped: " + list);
	}

	return loaded;
}
