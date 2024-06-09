#include "Texture.h"

//Man, one include only header only libraries make me want to just compile it into a static library
#define STB_IMAGE_IMPLEMENTATION
#include "../External/stb_image.h"

/*
	A note about STB Image
	I've tried to write my own custom allocators for STBI_MALLOC and such
	Using a persistant buffer that isn't allocated and freed with each image load
	Realistically this maybe saves 5-10% of the loading time for large textures after testing
	The way STB expects you to rewrite allocators seems prone to issues though
	And requires probably like 40-50Mb of RAM to be constantly reserved for this
	It just isn't worth it
*/

void Texture::bind(TextureLocations loc)
{
	if (!valid)
	{
		error("Texture::bind - Invalid texture cannot be used! " + name);
		return;
	}
	glActiveTexture(GL_TEXTURE0 + loc);
	glBindTexture(textureType, handle);
}

GLenum getTextureFormatEnum(int channels, bool hdrSpecific)
{
	switch (channels)
	{
		case 1: return hdrSpecific ? GL_R16F : GL_RED;
		case 2: return hdrSpecific ? GL_RG16F : GL_RG;
		case 3: return hdrSpecific ? GL_RGB16F : GL_RGB;
		case 4: return hdrSpecific ? GL_RGBA16F : GL_RGBA;
	}

	return 0;
}

TextureManager::TextureManager()
{
	//16 megabytes, if any textures are larger we can upsize it later
	lowDynamicRangeScratchpadSize = 1024 * 1024 * 16;
	lowDynamicRangeTextureScratchpad = new unsigned char[lowDynamicRangeScratchpadSize];
}

TextureManager::~TextureManager()
{
	for (unsigned int a = 0; a < textures.size(); a++)
		delete textures[a];

	lowDynamicRangeScratchpadSize = 0;
	delete[] lowDynamicRangeTextureScratchpad;
}

Texture::Texture()
{
	glGenTextures(1, &handle);
}

Texture::~Texture()
{
	glDeleteTextures(1, &handle);
}

Texture* TextureManager::createTexture(std::string filePath, bool makeMipmaps)
{
	scope("TextureManager::createTexture (non-array)");

	//Do a check to see if this texture was already loaded
	std::string textureName = getFileFromPath(filePath);

	for (unsigned int a = 0; a < textures.size(); a++)
	{
		if (textures[a]->name == textureName)
		{
			return textures[a];
		}
	}

	//No texture by that name exists, create a new one...
	Texture* ret = new Texture();
	ret->name = textureName;
	ret->textureType = GL_TEXTURE_2D;
	ret->layers = 1;

	//Check if file is valid and get dimensions
	stbi_info(filePath.c_str(), &ret->width, &ret->height, &ret->channels);

	debug("Loading texture " + filePath + " Dimensions: " + 
		std::to_string(ret->width) + "/" + 
		std::to_string(ret->height) + "/" + 
		std::to_string(ret->channels));

	if (ret->width < 1 || ret->height < 1 || ret->channels < 1)
	{
		error("Could not load texture " + filePath);
		delete ret;
		return 0;
	}

	if (ret->channels > 4)
	{
		error("Cannot load textures with more than 4 color channels, file: " + filePath);
		delete ret;
		return 0;
	}

	void* data = 0;

	//High or low dynamic range texture
	ret->isHDR = stbi_is_hdr(filePath.c_str());
	if (!ret->isHDR)
		data = stbi_load(filePath.c_str(), &ret->width, &ret->height, &ret->channels,0);
	else
		data = stbi_loadf(filePath.c_str(), &ret->width, &ret->height, &ret->channels, 0);

	if (!data)
	{ 
		error("Error processing image " + filePath);
		delete ret;
		return 0;
	}

	//Actually pass pixel data to OpenGL / graphics card
	glBindTexture(ret->textureType, ret->handle);
	glTexImage2D(
		ret->textureType,
		0,
		getTextureFormatEnum(ret->channels, ret->isHDR),
		ret->width,
		ret->height,
		0,
		getTextureFormatEnum(ret->channels, false),
		ret->isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
		data);

	//STBI calls malloc on our data in stbi_load(f) and this just calls free
	stbi_image_free(data);

	if (makeMipmaps)
	{
		ret->hasMipmaps = true;
		glGenerateMipmap(ret->textureType);
	}

	ret->valid = true;

	glBindTexture(ret->textureType, 0);

	ret->index = textures.size();
	textures.push_back(ret);
	return ret;
}

Texture* TextureManager::createBlankTexture(unsigned int width, unsigned int height, unsigned int channels, unsigned int layers,std::string name)
{
	//Since these are only used internally, i.e. as a place to render shadow data to
	//There's no real need for them to have user readable names
	if (name != "")
	{
		//Just in case it does though, make sure it doesn't already exist
		for (unsigned int a = 0; a < textures.size(); a++)
		{
			if (textures[a]->name == name)
			{
				return textures[a];
			}
		}
	}

	Texture* ret = new Texture();
	ret->textureType = (layers == 1) ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
	ret->width = width;
	ret->height = height;
	ret->channels = channels;
	ret->layers = layers;
	ret->name = name;
	ret->hasMipmaps = false;

	glBindTexture(ret->textureType, ret->handle);
	if (layers == 1)
	{
		glTexImage2D(
			ret->textureType,
			0,
			getTextureFormatEnum(ret->channels, ret->isHDR),
			ret->width,
			ret->height,
			0,
			getTextureFormatEnum(ret->channels, false),
			ret->isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
			(void*)0);
	}
	else
	{
		glTexImage3D(
			ret->textureType,
			0,
			getTextureFormatEnum(ret->channels, ret->isHDR),
			ret->width,
			ret->height,
			ret->layers,
			0,
			getTextureFormatEnum(ret->channels, false),
			ret->isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
			(void*)0);
	}

	ret->index = textures.size();
	textures.push_back(ret);
	return ret;
}

Texture *TextureManager::createTexture(unsigned int desiredLayers, std::string name, bool makeMipmaps)
{	
	scope("TextureManager::CreateTexture (array?)");
	if (name.length() < 1)
	{
		error("Name your textures!");
		return 0;
	}

	//Make sure we don't already have a texture by that name
	for (unsigned int a = 0; a < textures.size(); a++)
	{
		if (textures[a]->name == name)
		{
			/*
				Check to make sure you didn't get a preexisting texture back
				Before you call addLayer on the return value
			*/
			return textures[a];
		}
	}

	//Texture cannot be used until you load desiredLayers layers with addLayer
	Texture* ret = new Texture();
	ret->textureType = (desiredLayers == 1) ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
	ret->layers = desiredLayers;
	ret->name = name;
	ret->hasMipmaps = makeMipmaps;

	ret->index = textures.size();
	textures.push_back(ret);
	return ret;
}

void Texture::addLayer(std::string filePath)
{
	scope("Texture::addLayer");

	if (textureType != GL_TEXTURE_2D_ARRAY)
	{
		error("This is not an array texture!");
		return;
	}

	if (currentLayer >= layers)
	{
		error("Adding more layers to a texture than it was constructed for.");
		return;
	} 

	if (currentChannel != 0)
	{
		//Multiple components stack to make one layer, and multiple layers can stack to make one array texture
		//Someone tried to add a component to one layer, then add an entirely new layer before the last was done
		error("You can't call addLayer if you're in the middle of addComponent calls");
		return;
	}

	//Check if file is valid and get dimensions
	int readWidth, readHeight, readChannels;
	stbi_info(filePath.c_str(), &readWidth,&readHeight,&readChannels);

	if (readWidth < 1 || readHeight < 1 || readChannels < 1)
	{
		error("Could not load texture " + filePath);
		return;
	}

	if (readChannels > 4)
	{
		error("Cannot load textures with more than 4 color channels, file: " + filePath);
		return;
	}

	//High or low dynamic range texture
	bool readHDR = stbi_is_hdr(filePath.c_str());

	//If this is the first texture, let it set the dimensions for the whole array
	if (currentLayer == 0)
	{
		width = readWidth;
		height = readHeight;
		channels = readChannels;
		isHDR = readHDR;

		//Allocate all the space for all the layers of the texture 
		glBindTexture(textureType, handle);
		glTexImage3D(
			textureType,
			0,
			getTextureFormatEnum(channels, isHDR),
			width,
			height,
			layers,
			0,
			getTextureFormatEnum(channels, false),
			isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
			(void*)0);
	}
	//Successive texture loads need to have same dimensions as what's already in the array
	else if (width != readWidth || height != readHeight || channels != readChannels || readHDR != isHDR)
	{
		error(filePath + " did not have same dimensions as other textures in array " + name);
		return;
	}

	debug("Loading texture " + filePath + " Dimensions: " +
		std::to_string(width) + "/" +
		std::to_string(height) + "/" +
		std::to_string(channels));

	void* data = 0;

	if (!isHDR)
		data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
	else
		data = stbi_loadf(filePath.c_str(), &width, &height, &channels, 0);

	if (!data)
	{
		error("Error processing image " + filePath);
		return;
	}

	//Actually pass pixel data to OpenGL / graphics card
	glBindTexture(textureType, handle);
	glTexSubImage3D(
		textureType,
		0,
		0,
		0,
		currentLayer,
		width,
		height,
		1,
		getTextureFormatEnum(channels, false),
		isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
		data
	);

	//STBI calls malloc on our data in stbi_load(f) and this just calls free
	stbi_image_free(data);

	currentLayer++;

	//We did it! All intended layers of the texture array have been loaded
	if (currentLayer == layers)
	{
		if (hasMipmaps)
			glGenerateMipmap(textureType);

		valid = true;
	}

	glBindTexture(textureType, 0);
}

void TextureManager::addComponent(Texture* target, std::string filePath, int desiredTotalChannels)
{
	scope("TextureManager::addComponent");

	if (!target)
	{
		error("No texture specified");
		return;
	}

	if (target->channels == 0)
	{
		if (desiredTotalChannels < 1 || desiredTotalChannels > 4)
		{
			error("This is the first addition to texture " + target->name + " please set desired number of channels");
			return;
		}
		else
			target->channels = desiredTotalChannels;
	}

	if (target->isHDR)
	{
		error("This function does not work on HDR textures Name: " + target->name + " File: " + filePath);
		return;
	}

	if (target->valid)
	{
		error("This is already a complete texture! Name: " + target->name + " File: " + filePath);
		return;
	}

	if (scratchPadUserID != -1 && scratchPadUserID != target->index)
	{
		std::string name = "";
		if (scratchPadUserID < textures.size())
			name = textures[scratchPadUserID]->name;

		error("A different texture is currently being created " + name);
		return;
	}

	//Make sure no other textures can have components added until this texture('s layer) is done
	scratchPadUserID = target->index;

	//Check if file is valid and get dimensions
	int readWidth, readHeight, readChannels;
	stbi_info(filePath.c_str(), &readWidth, &readHeight, &readChannels);

	//Very first addition sets the dimensions for the rest of the texture
	if (target->currentLayer == 0 && target->currentChannel == 0)
	{
		target->width = readWidth;
		target->height = readHeight;
		//channels was set at the start of this function

		//Allocate all the space for all the layers of the texture 
		glBindTexture(target->textureType, target->handle);
		if (target->textureType == GL_TEXTURE_2D_ARRAY)
		{
			glTexImage3D(
				target->textureType,
				0,
				getTextureFormatEnum(target->channels, false),
				target->width,
				target->height,
				target->layers,
				0,
				getTextureFormatEnum(target->channels, false),
				GL_UNSIGNED_BYTE,
				(void*)0);
		}
		else
		{
			glTexImage2D(
				target->textureType,
				0,
				getTextureFormatEnum(target->channels, false),
				target->width,
				target->height,
				0,
				getTextureFormatEnum(target->channels, false),
				GL_UNSIGNED_BYTE,
				(void*)0
			);
		}

		//Double check to make sure scratchPad can support the full size of this texture('s layer):
		if (lowDynamicRangeScratchpadSize < target->width * target->height * target->channels)
		{
			//This should never really happen
			delete[] lowDynamicRangeTextureScratchpad;
			lowDynamicRangeScratchpadSize = target->width * target->height * target->channels;
			lowDynamicRangeTextureScratchpad = new unsigned char[lowDynamicRangeScratchpadSize];
		}
	}
	//Subsequent addition failed to match addition of previous files read for this texture
	else if (target->width != readWidth || target->height != readHeight)
	{
		error("Image " + filePath + " did not have same dimensions as rest of " + target->name);
		scratchPadUserID = -1; //Unlock scratchpad
		return;
	}

	debug("Loading texture " + filePath + " Dimensions: " +
		std::to_string(target->width) + "/" +
		std::to_string(target->height) + "/" +
		std::to_string(target->channels));

	stbi_uc *data = stbi_load(filePath.c_str(), &readWidth, &readHeight, &readChannels, 0);

	if (!data)
	{
		error("Error processing image " + filePath);
		scratchPadUserID = -1; //Unlock scratchpad
		return;
	}
	
	/*
		Copy each pixel from the first / only channel of the file we just loaded
		into the scratch pad such that we fill in one channel of the scratchPad per call to addComponent
	*/
	std::cout << "Cur channel: " << target->currentChannel << "\n";
	for (int a = 0; a < target->width * target->height; a++)
		lowDynamicRangeTextureScratchpad[a * target->channels + target->currentChannel] = data[a * readChannels];

	stbi_image_free(data);

	target->currentChannel++;
	//All channels for this layer (or the entire texture if it's not an array texture) have been loaded
	if (target->currentChannel == target->channels)
	{
		//Actually pass pixel data to OpenGL / graphics card
		glBindTexture(target->textureType, target->handle);
		if (target->textureType == GL_TEXTURE_2D_ARRAY)
		{
			glTexSubImage3D(
				target->textureType,
				0,
				0,
				0,
				target->currentLayer,
				target->width,
				target->height,
				1,
				getTextureFormatEnum(target->channels, false),
				GL_UNSIGNED_BYTE,
				lowDynamicRangeTextureScratchpad
			);
		}
		else
		{
			glTexSubImage2D(
				target->textureType,
				0,
				0,
				0,
				target->width,
				target->height,
				getTextureFormatEnum(target->channels, false),
				GL_UNSIGNED_BYTE,
				lowDynamicRangeTextureScratchpad
			);
		}

		scratchPadUserID = -1; //Unlock scratchpad

		target->currentLayer++;

		//Reset this for the next layer, or allow adding entire layers again
		target->currentChannel = 0;

		//We did it! All intended layers of the texture array have been loaded
		if (target->currentLayer == target->layers)
		{
			if (target->hasMipmaps)
				glGenerateMipmap(target->textureType);

			target->valid = true;
		}

		glBindTexture(target->textureType, 0);
	}
}

void Texture::setFilter(GLenum magFilter, GLenum minFilter)
{
	if ((minFilter == GL_LINEAR_MIPMAP_LINEAR 
		|| minFilter == GL_LINEAR_MIPMAP_NEAREST
		|| minFilter == GL_NEAREST_MIPMAP_LINEAR
		|| minFilter == GL_NEAREST_MIPMAP_NEAREST) && !hasMipmaps)
	{
		error("Tried to set mipmap filter for " + name + " which has none.");
		return;
	}

	glBindTexture(textureType, handle);
	glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, minFilter);
	glBindTexture(textureType, 0);
}

void Texture::setWrapping(GLenum wrapping)
{
	glBindTexture(textureType, handle);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_S, wrapping);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_T, wrapping);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_R, wrapping);
	glBindTexture(textureType, 0);
}

void Texture::setWrapping(GLenum wrapS, GLenum wrapT, GLenum wrapR)
{
	glBindTexture(textureType, handle);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_R, wrapR);
	glBindTexture(textureType, 0);
}