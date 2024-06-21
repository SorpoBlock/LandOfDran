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

void Texture::bind(TextureLocations loc) const
{
	if (!valid)
	{
		error("Texture::bind - Invalid texture cannot be used! " + name);
		return;
	}
	glActiveTexture(GL_TEXTURE0 + loc);
	glBindTexture(textureType, handle);
}

void TextureManager::allocateTexture(Texture* target)
{
	int getMaxMipLevels = std::max(target->width, target->height);
	getMaxMipLevels = (int)log2(getMaxMipLevels);
	if (getMaxMipLevels <= 0)
		getMaxMipLevels = 1;

	glBindTexture(target->textureType, target->handle);
	if (target->textureType == GL_TEXTURE_2D)
	{
		glTexStorage2D(target->textureType, getMaxMipLevels
			, getSizedFormatEnum(target->channels, target->isHDR)
			, target->width, target->height);
	}
	else
	{
		glTexStorage3D(target->textureType, getMaxMipLevels,
			getSizedFormatEnum(target->channels, target->isHDR),
			target->width, target->height, target->layers);
	}
}

GLenum getSizedFormatEnum(int channels, bool hdr)
{
	switch (channels)
	{
		case 1: return hdr ? GL_R16F : GL_R8;
		case 2: return hdr ? GL_RG16F : GL_RG8;
		case 3: return hdr ? GL_RGB16F : GL_RGB8;
		case 4: return hdr ? GL_RGBA16F : GL_RGBA8;
	}

	return 0;
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
	//16 megabytes, if any textures are larger we will upsize it later
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

Texture* TextureManager::createTexture(const std::string &filePath, bool makeMipmaps)
{
	scope("TextureManager::createTexture (non-array)");

	//Do a check to see if this texture was already loaded
	std::string textureName = getFileFromPath(filePath);

	for (unsigned int a = 0; a < textures.size(); a++)
	{
		if (textures[a]->name == textureName)
		{
			textures[a]->usages++;
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
	ret->usages++;
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
				textures[a]->usages++;
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

	allocateTexture(ret);

	ret->index = textures.size();
	textures.push_back(ret);
	ret->usages++;
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
		if (textures[a]->name == "")
			continue;

		if (textures[a]->name == name)
		{
			/*
				Check to make sure you didn't get a preexisting texture back
				Before you call addLayer on the return value
			*/
			textures[a]->usages++;
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
	ret->usages++;
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

	debug("Loading texture " + filePath + " Dimensions: " +
		std::to_string(readWidth) + "/" +
		std::to_string(readHeight) + "/" +
		std::to_string(readChannels));

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
	else if (width != readWidth || height != readHeight)
	{
		error(filePath + " did not have same dimensions as other textures in array " + name);
		return;
	}

	void* data = 0;

	if (!isHDR)
		data = stbi_load(filePath.c_str(), &width, &height, &readChannels, channels);
	else
		data = stbi_loadf(filePath.c_str(), &width, &height, &readChannels, channels);

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

void TextureManager::finishLayer(Texture * target)
{
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

void TextureManager::addEmptyComponent(Texture* target)
{
	scope("TextureManager::addEmptyComponent");

	if (!target)
	{
		error("No texture specified");
		return;
	}

	if (target->channels == 0)
	{
		error("Empty component cannot be the first component of the first layer of a texture (array)");
		return;
	}

	if (target->isHDR)
	{
		error("This function does not work on HDR textures Name: " + target->name);
		return;
	}

	if (target->valid)
	{
		error("This is already a complete texture! Name: " + target->name);
		return;
	}

	finishLayer(target);
}

void Texture::markForCleanup()
{
	usages--;

	if (usages < 0)
		error("Texture " + name + " somehow has *negative* references now!");
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
	scratchPadUserID = (int)target->index; 

	//Check if file is valid and get dimensions
	int readWidth, readHeight, readChannels;
	stbi_info(filePath.c_str(), &readWidth, &readHeight, &readChannels);

	debug("Loading texture " + filePath + " Dimensions: " +
		std::to_string(readWidth) + "/" +
		std::to_string(readHeight) + "/" +
		std::to_string(readChannels));

	//Very first addition sets the dimensions for the rest of the texture
	if (target->currentLayer == 0 && target->currentChannel == 0)
	{
		target->width = readWidth;
		target->height = readHeight;
		//channels was set at the start of this function

		//Allocate all the space for all the layers of the texture 
		allocateTexture(target);

		//Double check to make sure scratchPad can support the full size of this texture('s layer):
		if (lowDynamicRangeScratchpadSize < (unsigned int)(target->width * target->height * target->channels))
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
	for (int a = 0; a < target->width * target->height; a++)
		lowDynamicRangeTextureScratchpad[a * target->channels + target->currentChannel] = data[a * readChannels];

	stbi_image_free(data);

	finishLayer(target);
}

void Texture::setFilter(GLenum magFilter, GLenum minFilter) const
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

void Texture::setWrapping(GLenum wrapping) const
{
	setWrapping(wrapping, wrapping, wrapping);
}

void Texture::setWrapping(GLenum wrapS, GLenum wrapT, GLenum wrapR) const
{
	glBindTexture(textureType, handle);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(textureType, GL_TEXTURE_WRAP_R, wrapR);
	glBindTexture(textureType, 0);
}

void TextureManager::garbageCollect()
{
	auto iter = textures.begin();

	while (iter != textures.end())
	{
		Texture* t = *iter;

		if (t->usages < 1)
		{
			delete t;
			iter = textures.erase(iter);
		}
		else
			++iter;
	}
}

void TextureManager::cleanupDecals()
{
	currentDecalCount = 0;
	if (decals)
	{
		delete decals;
		decals = nullptr;
	}
}

void TextureManager::allocateForDecals(unsigned int dimensions, unsigned int maxEntries)
{
	if (decals)
		cleanupDecals();

	decals = new Texture();
	decals->textureType = GL_TEXTURE_2D_ARRAY;

	//A name isn't really needed, as it's the only texture not in the textures vector.
	decals->name = "decals";

	decals->width = dimensions;
	decals->height = dimensions;

	//How many decals a we can have on a given server
	decals->layers = maxEntries;

	//Red, Green, Blue, Alpha
	decals->channels = 4;

	allocateTexture(decals);

	std::fill(lowDynamicRangeTextureScratchpad, lowDynamicRangeTextureScratchpad + decals->width * decals->height, 0);

	glTexSubImage3D(decals->textureType, 0, 0, 0, 0, decals->width, decals->height, 1,
		getTextureFormatEnum(decals->channels, false),
		decals->isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
		lowDynamicRangeTextureScratchpad);

	currentDecalCount++;
}

void TextureManager::addDecal(const std::string &filePath,int id)
{
	scope("TextureManager::addDecal");

	if (id < 0 || (unsigned)id >= decals->layers)
	{
		error("Decal ID " + std::to_string(id) + " is beyond max decals " + std::to_string(decals->layers));
		return;
	}

	//Check if file is valid and get dimensions
	int readWidth, readHeight, readChannels;
	stbi_info(filePath.c_str(), &readWidth, &readHeight, &readChannels);

	if (readWidth == 0 || readHeight == 0)
	{
		error("Could not open image " + filePath);
		return;
	}

	if (readWidth != 256 || readHeight != 256)
	{
		error("Decal file " + filePath + " did not have dimensions of 256x256");
		return;
	}

	debug("Loading texture " + filePath + " Dimensions: " +
		std::to_string(decals->width) + "/" +
		std::to_string(decals->height) + "/4");

	//Last parameter is 4 to force an alpha channel, if there wasn't one the image will be opaque
	stbi_uc* data = stbi_load(filePath.c_str(), &readWidth, &readHeight, &readChannels, 4);

	if (!data)
	{
		error("Could not load image " + filePath);
		return;
	}

	//User has 256x256 decals set in graphics settings, no downsizing needed!
	if (decals->width == 256)
	{
		glTexSubImage3D(decals->textureType, 0, 0, 0, id, decals->width, decals->height, 1,
			getTextureFormatEnum(decals->channels, false),
			decals->isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
			data);
	}
	else
	{
		//Make sure scratch pad isn't in use
		//There's no point in locking it here, since it'll be unlocked by the end of the function anyway
		if (scratchPadUserID != -1)
		{
			error("Scratch pad was in use from adding component!");
			return;
		}

		if (decals->width == 128)
		{
			for (unsigned int x = 0; x < 128; x++)
			{
				for (unsigned int y = 0; y < 128; y++)
				{
					int sourceX = x * 2;
					int sourceY = y * 2;

					//Red, Green, Blue, Alpha
					lowDynamicRangeTextureScratchpad[ (x + y * 128) * 4] =		data[ (sourceX + sourceY * 256) * 4];
					lowDynamicRangeTextureScratchpad[((x + y * 128) * 4) + 1] = data[((sourceX + sourceY * 256) * 4) + 1];
					lowDynamicRangeTextureScratchpad[((x + y * 128) * 4) + 2] = data[((sourceX + sourceY * 256) * 4) + 2];
					lowDynamicRangeTextureScratchpad[((x + y * 128) * 4) + 3] = data[((sourceX + sourceY * 256) * 4) + 3];
				}
			}
		}
		else //64x64
		{
			for (unsigned int x = 0; x < 64; x++)
			{
				for (unsigned int y = 0; y < 64; y++)
				{
					int sourceX = x * 4;
					int sourceY = y * 4;

					//Red, Green, Blue, Alpha
					lowDynamicRangeTextureScratchpad[ (x + y * 64) * 4] =	   data[ (sourceX + sourceY * 256) * 4];
					lowDynamicRangeTextureScratchpad[((x + y * 64) * 4) + 1] = data[((sourceX + sourceY * 256) * 4) + 1];
					lowDynamicRangeTextureScratchpad[((x + y * 64) * 4) + 2] = data[((sourceX + sourceY * 256) * 4) + 2];
					lowDynamicRangeTextureScratchpad[((x + y * 64) * 4) + 3] = data[((sourceX + sourceY * 256) * 4) + 3];
				}
			}
		}

		glTexSubImage3D(decals->textureType, 0, 0, 0, id, decals->width, decals->height, 1,
			getTextureFormatEnum(decals->channels, false),
			decals->isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE,
			lowDynamicRangeTextureScratchpad);
	}

	stbi_image_free(data);

	currentDecalCount++;
}

void TextureManager::finalizeDecals()
{
	//Already been called
	if (decals->valid)
		return;

	glBindTexture(decals->textureType, decals->handle);
	glGenerateMipmap(decals->textureType);
	decals->valid = true;
	glBindTexture(decals->textureType, 0);
	
	decals->bind(DecalArray);
	//This line makes sure that the next texture op, including loading textures, won't unset our decals texture
	glActiveTexture(GL_TEXTURE0);
}