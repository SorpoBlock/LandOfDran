#include "Material.h"

Material::Material(std::string filePath,TextureManager * textures)
{
	scope("Material::Material");

	std::string name = getFileFromPath(filePath.c_str());

	std::ifstream materialDescriptor(filePath.c_str());

	if (!materialDescriptor.is_open())
	{
		error("Could not open " + filePath);
		return;
	}

	/*
		At the moment, a material has the following things for PBR:
		Albedo - aka color
		Normal Map
		Metalness
		Roughness
		Ambient Occlusion

		When making a material you may use or not use any of these
		The only restriction atm is that you can't use metal/rough/ao without also using either albedo and/or normal map
	*/
	
	//Remains "" if not used
	std::string albedoPath = "";
	std::string normalPath = "";
	std::string metalPath = "";
	std::string roughPath = "";
	std::string aoPath = "";

	std::string line = "";
	while (!materialDescriptor.eof())
	{
		getline(materialDescriptor, line);

		if (line.length() < 1)
			continue;

		if (line[0] == '#')
			continue;

		size_t tabPos = line.find("\t");
		if (tabPos == std::string::npos)
		{
			error("Invalid line in file " + filePath);
			return;
		}
		
		//Each line in this file should contain two words separated by a tab
		//The first is the type of texture i.e. 'albedo'
		//The second is a relative path to the image file
		std::string type = line.substr(0, tabPos);
		std::string path = line.substr(tabPos + 1, std::string::npos);

		if (type.length() < 1)
		{
			error("No texture type specified " + filePath);
			return;
		}

		if (path.length() < 1)
		{
			error("Invalid path specified " + filePath);
			return;
		}

		//You can optionally specify a name for the material for use with Lua or whatever later
		//It can also prevent reusing the same material
		if (type == "name")
			name = path;
		else if (type == "albedo")
			albedoPath = path;
		else if (type == "normal")
			normalPath = path;
		else if (type == "metalness")
			metalPath = path;
		else if (type == "roughness")
			roughPath = path;
		else if (type == "occlusion")
			aoPath = path;
		else
		{
			error("Invalid material texture type: " + type + " file: " + filePath);
			return;
		}
	}

	int howManyLayers = 0;
	if (albedoPath.length() > 0)
		howManyLayers++;
	if (normalPath.length() > 0)
		howManyLayers++;
	if (metalPath.length() > 0 || roughPath.length() > 0 || aoPath.length() > 0)
		howManyLayers++;

	int currentLayer = 0;

	PBRArrayTexture = textures->createTexture(howManyLayers, name);

	//In the super unlikely event a texture already exists with this exact name, use it instead?
	if (PBRArrayTexture->isValid())
	{
		valid = true;
		return;
	}

	if (albedoPath.length() > 0)
	{
		useAlbedo = currentLayer;
		++currentLayer;
		PBRArrayTexture->addLayer(albedoPath);
	}

	if (normalPath.length() > 0)
	{
		useNormal = currentLayer;
		++currentLayer;
		PBRArrayTexture->addLayer(normalPath);
	}

	//No point in incrementing currentLayer beyond this point...
	//Create the final layer with various masks used for PBR rendering

	//Bail early, no metalness/roughness/occlusion data, no need for a 3rd layer
	if (howManyLayers == currentLayer)
	{
		if (PBRArrayTexture->isValid())
			valid = true;
	}

	//Metalness
	if (metalPath.length() > 0)
	{
		useMetalness = currentLayer;
		textures->addComponent(PBRArrayTexture, metalPath);
	}
	else
		textures->addEmptyComponent(PBRArrayTexture);

	//Ambient occlusion
	if (aoPath.length() > 0)
	{
		useOcclusion = currentLayer;
		textures->addComponent(PBRArrayTexture, aoPath);
	}
	else
		textures->addEmptyComponent(PBRArrayTexture);

	//Roughness
	if (roughPath.length() > 0)
	{
		useRoughness = currentLayer;
		textures->addComponent(PBRArrayTexture, roughPath);
	}
	else
		textures->addEmptyComponent(PBRArrayTexture);

	if(PBRArrayTexture->isValid())
		valid = true;
}

Material::~Material()
{
	if (PBRArrayTexture)
		PBRArrayTexture->markForCleanup();
}

void Material::use(ShaderManager* shaders)
{
	if (!valid)
		return;

	shaders->globalUniforms.useAlbedo =		useAlbedo;
	shaders->globalUniforms.useNormal =		useNormal;
	shaders->globalUniforms.useMetalness =	useMetalness;
	shaders->globalUniforms.useRoughness =	useRoughness;
	shaders->globalUniforms.useAO =			useOcclusion;

	shaders->updateUniformBlock();

	PBRArrayTexture->bind(PBRArray);
}
