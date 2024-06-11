#pragma once

#include "../LandOfDran.h"
#include "Texture.h"
#include "ShaderSpecification.h"

class Material
{
	/*
		Does PBRArrayTexture contain an albedo layer ? 
		-1 if not, otherwise points to the layer its on
	*/
	int useAlbedo = -1;

	/*
		Does PBRArrayTexture contain a normal map layer ? 
		-1 if not, otherwise points to the layer its on
	*/
	int useNormal = -1;

	/*
		Does PBRArrayTexture contain a MOHR layer with a metalness channel ? 
		-1 if not, otherwise points to the layer its on
		metalness, roughness, and occlusion will always be on the same layer, though they might not all be used
	*/
	int useMetalness = -1;

	/*
		Does PBRArrayTexture contain a MOHR layer with a roughness channel ?
		-1 if not, otherwise points to the layer its on
		metalness, roughness, and occlusion will always be on the same layer, though they might not all be used
	*/
	int useRoughness = -1;

	/*
		Does PBRArrayTexture contain a MOHR layer with a occlusion channel ?
		-1 if not, otherwise points to the layer its on
		metalness, roughness, and occlusion will always be on the same layer, though they might not all be used
	*/
	int useOcclusion = -1;

	//Used in 3d model description files, or possibly Lua
	std::string name = "";

	Texture* PBRArrayTexture = 0;

	bool valid = false;

	public:

	/*
		Sets PBRArrayTexture as the active PBRArray texture
		Passes other uniforms describing the texture to shaders
	*/
	void use(ShaderManager * shaders) const;

	/*
		Pass a filepath to a text file containing a description of this material
		and the filepaths of the indivdual textures it has
	*/
	Material(std::string filePath, TextureManager* textures);

	~Material();
};