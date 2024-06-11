#pragma once

#include "../LandOfDran.h"
#include "Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

class Model;

class Mesh
{
	friend class Model;

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

	Mesh(aiMesh const * const src);
	~Mesh();

	public:

	//Render all instances of this particular mesh
	void render(ShaderManager* graphics, bool useMaterials = true) const;
};

class Model
{
	//Every mesh the model has stored in a way that doesn't require recusion to access
	std::vector<Mesh*> allMeshes;

	public:

};
