#pragma once

#include "../LandOfDran.h"
#include "Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtx/matrix_decompose.hpp>

//How many instances for an instanced mesh we allocate space for at a time, don't set to 0
#define InstanceBufferPageSize 1000

/*
	Determines what order the various per-vertex variables
	are passed to the OpenGL shaders i.e. the 0 in:
	layout(location = 0) in vec3 positions;
	Used in glVertexAttribPointer
*/
enum LayoutSlot
{
	ModelSpace = 0,			//vec3 - Per-vertex position in model space
	NormalVector = 1,		//vec3 - Normal in model space
	TangentVector = 2,		//vec3
	BitangentVector = 3,	//vec3
	TextureCoords = 4,		//vec2 - uvs
	PreColor = 5,			//vec4 - Optional per-brick or per-mesh color before material application (per-instance)
	InstanceFlags = 6,		//int  - Contains additional per-instance info such as decal used, or mouse-picking info
	ModelTransform = 7		//mat4 - Contains the model matrix if rendering instanced models (per-instance)
};

#define MeshFlag_UsePickingColor  256     //Render the mesh with colors for mouse picking
//Bits 0-7 would then be the mouse picking ID if that flag is enabled
#define MeshFlag_UseDecal		  131072  //Should we render a decal on top of our current material
//Bits 9-16 would then be the decal ID
#define MeshFlag_SkipCameraMatrix 262144  //Skip camera projection and view matricies in vertex shader

class Mesh;
class Model;
class Node;

/*
	Contains the information needed to render one instance of a Model
	One set of data for each instanced layout binding point for each mesh of the model
*/
class ModelInstance
{	 
	Model* type = nullptr;

	/*
 		These allow you to manually tweak the rotation of a given node
   		For instance if you want to make a player's head tilt when they look up or down
     	*/
	std::vector<glm::quat>  NodeRotationFixes;
	//If NodeRotationFixes should be used for that given node
	std::vector<bool> 	UseNodeRotationFix;

	//The transform for the entire model instance
	glm::mat4 wholeModelTransform = glm::mat4(1.0);

	/*
		These are calculated in calculateMeshTransforms per Mesh based on:
  		The node hierarchy as imported from Assimp with any baked in hierarchical transformations
    	Any currently playing animations
      	Any manually applied NodeRotationFixes
		wholeModelTransform
 	*/
	std::vector<glm::mat4>		MeshTransforms;
	std::vector<unsigned int>	MeshFlags;
	std::vector<glm::vec4>		MeshColors;

	//Were any of the above properties changed since last frame
	//It's assumed any transform update would affect the entire model...
	bool transformUpdated = true;

	//If anything at all has been updated, any mesh's transform, flags, or colors, since last frame
	bool anythingUpdated = true;

	//... while these are per-mesh
	std::vector<bool> flagsUpdated;
	std::vector<bool> colorsUpdated;

	//What offset to use for Mesh buffers for instanced data when calling performMeshBufferUpdates
	unsigned int bufferOffset = 0;

	public:

	//Change the position/scale/rotation for the whole model instance
	void setModelTransform(glm::mat4 transform);
	//For little tweaks like making a player's head swivel up and down based on where they look
	void setNodeRotation(int nodeId,glm::quat rotation);
	//Set a flag or a custom color on a single mesh instance of the model instance
	void setFlags(int meshId,unsigned int flags);
	void setColor(int meshId,glm::vec4 color);

	/*
		Calculates the transform of each indivdual node based on things, see above
		Call once per frame if the object is animated or has moved
	*/
	void calculateMeshTransforms(glm::mat4 currentTransform = glm::mat4(1.0),Node * currentNode = 0);

	/*
		Calls glBufferSubData on mesh buffers that need updating, and sets updated flags to false
		Call this once per instance per frame, after any calls to set* but before rendering
	*/
	void performMeshBufferUpdates();

	//Calls calculateMeshTransforms and performMeshBufferUpdates
	void update();

	ModelInstance(Model * _type); 
	~ModelInstance();
};

class Mesh
{
	friend class Model;
	friend class ModelInstance;

	//How many instances worth of space we've allocated in each of the instanced buffers
	unsigned int instancesAllocated = 0;

	//This vector's members are shared between other Mesh's of the same Model
	std::vector<ModelInstance*> instances;

	//Assimp can specifiy a material for this particular mesh
	Material* material = nullptr;

	//If the model came with normals when loaded
	bool hasNormals = false;

	//Has both tangents *and* bitangents cause why would you have one but not the other?
	bool hasTangents = false;

	//Did the model come with texture coordinates
	bool hasUVs = false;

	//If everything loaded okay and this mesh can be used
	bool valid = false;

	//How many verticies (including duplicates) does this mesh have
	unsigned int vertexCount = 0;

	//Used for seeing what index to use when reading from vectors in ModelInstance for rendering
	unsigned int meshIndex = 0;

	/*
		Vertex array object that contains the mesh
		With verticies, and optionally normals, tangents, uvs, etc.
		Can be used for instanced or non-instanced rendering
	*/
	GLuint vao;

	//Buffers correspond to LayoutSlot
	GLuint buffers[8];

	/*
		Additional buffer that holds the index of the next vertex in the above buffers so
		repeat verticies don't need to be passed to the GPU multiple times
		In other words: GL_ELEMENT_ARRAY_BUFFER Indicies are 16 bit unsigned shorts
	*/
	GLuint indexBuffer;

	//Helper function to remove duplicated code from Mesh::Mesh. 
	//Cannot be used for index buffer. Make sure VAO is already bound
	void fillBuffer(LayoutSlot slot, void* data, int size, int elements);

	Mesh(aiMesh const * const src,Model const * const parent);
	~Mesh();

	public:

	//Render all instances of this particular mesh
	void render(ShaderManager* graphics, bool useMaterials = true) const;
};

class Node
{
	friend class Model;
	friend class ModelInstance;

	/*
		The transform a node will have, if no animations are playing
		Is overrided entirely if an animation is playing that affects that node
	*/
	glm::mat4 defaultTransform = glm::mat4(1.0);

	//Leaves of the tree
	std::vector<Mesh*> meshes;

	//Babies
	std::vector<Node*> children;

	//Node above
	Node* parent = nullptr;

	/*
		This is added by Assimp
		The matrix multiplication for each node goes:
		translate(rotationPivot) * rotate(rotation) * translate(-rotationPivot) * translate(pos) * higherNode...
	*/
	glm::vec3 rotationPivot;

	//Undoes some of Assimps importing silliness that creates a billion extra nodes, see stripSillyAssimpNodeNames
	void foldNodeInto(aiNode const * const src, Model * parent);

	//Used for setting properties through script
	std::string name = "";

	Node(aiNode const* const src, Model* parent);
	~Node();
};

class Model
{
	friend class ModelInstance;
	friend class Mesh;
	friend class Node;

	//Every mesh the model has stored in a way that doesn't require recusion to access
	std::vector<Mesh*> allMeshes;
	
	//Nodes for non-hierarchical access
	std::vector<Node*> allNodes;

	//Assigned to indivdual meshes...
	std::vector<Material*> allMaterials;

	//Entry point to recursive calculations
	Node* rootNode = nullptr;

	public:

	//Calls render on each mesh
	void render(ShaderManager* graphics,bool useMaterials = true) const;

	/*
		File path refers to a text file that describes where the actual model is
		Flags for how to load it, and other text files describing materials
	*/
	Model(std::string filePath, TextureManager* textures);

	~Model();
};
