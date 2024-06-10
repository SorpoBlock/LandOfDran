#pragma once

#include "../LandOfDran.h"
#include "ShaderSpecification.h"

/*
	Just remember that multiple channels/components can be combined to make one texture
	or one layer of an array texture, and multiple layers combine to make an array texture
	A possible structure for a PBR workflow would be like this:
	Texture *metal = textureManager.createTexture(3,"MaterialMetal");
	metal->addLayer("Albedo.png");			3 channels
	metal->addLayer("Normal.png");			3 channels
	metal->addComponent("Roughness.png");	1 channel
	metal->addComponent("Metalness.png");	1 channel
	metal->addComponent("Occlusion.png");	1 channel
*/

/*
	Returns GLenum to pass for internalFormat argument to texture functions
	value for format can be retrieved by setting hdrSpecific to false always
*/
GLenum getTextureFormatEnum(int channels, bool hdrSpecific);

class TextureManager;

/*
	A wrapper around an OpenGL texture with various constructors and helper functions to make life easier
*/
class Texture
{
	friend TextureManager;

	//What position in TextureManager::Array was this created with
	size_t index = 0;

	//Only used when combing multiple channels into a single multi-channel texture or layer of an array texture
	unsigned int currentChannel = 0;

	//Only used while creating an array texture from multiple files (i.e. PBR materials)
	unsigned int currentLayer = 0;

	//This class is capable of handling GL_TEXTURE_2D and most GL_TEXTURE_2D_ARRAY at the moment
	GLenum textureType = GL_TEXTURE_2D;

	//How many color channels does each layer have
	int channels = 0;

	//Were mipmaps made upon texture creation
	bool hasMipmaps = false;

	//If texture(s) was loaded properly
	bool valid = false;

	//Should be 1 unless textureType == GL_TEXTURE_2D_ARRAY
	unsigned int layers = 0;

	//Width in pixels
	int width = 0;

	//Height in pixels
	int height = 0;

	//Is it high dynamic range color, or normal 24bit color:
	bool isHDR = false;

	//Reference to the actual texture in OpenGL
	GLuint handle;

	/*
		Name allows us to return an already loaded texture instead of loading a duplicate 
		Keep it case-insensitive
	 
		Will either be the file name without folders
		i.e. 'add-ons/print_screen/screen.png' -> 'screen.png'
		or a material name from a text file describing materials
		i.e. 'add-ons/player_default/player.txt' -> perhaps 'playerfoot'
	*/
	std::string name = "";

	/*
		Makes this whole class kind of act like a shared_ptr
		Each TextureManager::createTexture adds one to this
		Each markForCleanup removes one from this
		When it's below 0 TextureManager::garbageCollect will destroy it
	*/
	int usages = 0;

	Texture();
	~Texture();

	Texture(const Texture&) = delete; //Disable copy constructor

	public:

	//Has this texture been put together succesfully, no missing files, and can be used for rendering
	bool isValid() const { return valid;  }

	/*
		Decrements usages by one
		Call TextureManager::garbageCollect at some point
	*/
	void markForCleanup();

	void setFilter(GLenum magFilter, GLenum minFilter);
	void setWrapping(GLenum wrapping);
	void setWrapping(GLenum wrapS, GLenum wrapT, GLenum wrapR);

	//Actually use the texture
	void bind(TextureLocations loc);

	//Loads one layer of a 2D texture array from a file
	void addLayer(std::string filePath);
};

/*
	Holds all the textures loaded in the game for resource management
	Keeps track of textures to make sure they can all be deallocated upon joining a new game
	Makes sure you don't load a texture from the same file twice
*/
class TextureManager
{
	/*
		This is used when trying to compile multiple one channel textures into
		a single result (non-array) texture with 2-4 channels
	*/
	unsigned char* lowDynamicRangeTextureScratchpad = 0;

	//How much memory has been allocated to lowDynamicRangeTextureScratchpad
	unsigned int lowDynamicRangeScratchpadSize = 0;

	//-1 if lowDynamicRangeTextureScratchpad is not being used
	//Otherwise points to the texture that is currently being constructed
	int scratchPadUserID = -1;

	std::vector<Texture*> textures;

	/*
		Called after adding the last channel/component of a given layer
		Finalizes the layer or the whole texture if its ready
	*/
	void finishLayer(Texture* target);

	/*
		Calls glTexImage2D or glTexImage3D
		Can pass pixel data or data can be left nullptr just to allocate
	*/
	void allocateTexture(Texture* target,void *data);

	public:

	/*
		Creates a non-array texture from a single image file and returns it
		Unless a texture from that file already exists, in which case the existing is returned
		Will detect if it is HDR or not HDR
	*/
	Texture *createTexture(std::string filePath,bool makeMipmaps = true);

	/*
		Create a 2d texture array, for use with things like PBR material workflows
		If desiredLayers > 1 it is an array texture, otherwise it is a GL_TEXTURE_2D
		You have to add each layer indivdually after using Texture::addLayer
	*/
	Texture* createTexture(unsigned int desiredLayers,std::string name, bool makeMipmaps = true);

	//Creates a texture with all values set to 0, can then be used for rendering to
	Texture* createBlankTexture(unsigned int width, unsigned int height, unsigned int channels, unsigned int layers,std::string name = "");

	/*
		Loads a one channel texture or the first channel of a multi channel texture and
		combines it with other reads in order to create a single texture (or layer) with 2-4 components
		Can be done on array textures and non - array textures, but must be LDR
		You need to set desiredTotalChannels only if this is the first thing you've added to this texture
	*/
	void addComponent(Texture *target,std::string filePath,int desiredTotalChannels = -1);

	/*
		Adds a channel to a layer filled entirely with undefined values
		Cannot be the first component of the first layer
	*/
	void addEmptyComponent(Texture* target);

	/*
		Destroys any textures with a usage count less than 1
	*/
	void garbageCollect();

	TextureManager();
	~TextureManager();
};

