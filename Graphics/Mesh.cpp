#include "Mesh.h"

void Mesh::fillBuffer(LayoutSlot slot, void* data, int size, int elements)
{
	glBindBuffer(GL_ARRAY_BUFFER, buffers[slot]);
	glEnableVertexAttribArray(slot);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glVertexAttribDivisor(slot, 0);
	glVertexAttribPointer(slot, elements, GL_FLOAT, GL_FALSE, 0, 0);
}

Mesh::Mesh(aiMesh const* const src)
{
	scope("Mesh::Mesh");

	debug("Loading mesh: " + std::string(src->mName.C_Str()));

	glGenVertexArrays(1, &vao);

	glGenBuffers(8, buffers);
	glGenBuffers(1, &indexBuffer);

	glBindVertexArray(vao);

	//We're going to allocate space and set properties for the 3 per-instance buffers now:

	//A vec4 RGBA color. One per instance of the mesh rendered.
	glBindBuffer(GL_ARRAY_BUFFER, buffers[PreColor]);
	glEnableVertexAttribArray(PreColor);
	glVertexAttribPointer(PreColor, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribDivisor(PreColor, 1);

	//A 32bit int that represents at least a few things:
	//Color-based mouse-picking IDs when mouse-picking
	//Whether to use a decal and if so which one
	//Various special effects and stuff 
	glBindBuffer(GL_ARRAY_BUFFER, buffers[InstanceFlags]);
	glEnableVertexAttribArray(InstanceFlags);
	glVertexAttribIPointer(InstanceFlags, 1, GL_INT, 0, (void*)0);
	glVertexAttribDivisor(InstanceFlags, 1);

	//In OpenGL you *can* pass a mat4 as a vertex buffer based layout binding
	//But you technically need to use 4 different layout locations to achieve it, 
	//as if each one passed one row of a 4x4 matrix
	//This is per-instance as well btw 
	glBindBuffer(GL_ARRAY_BUFFER, buffers[ModelTransform]);
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(ModelTransform + i);
		glVertexAttribPointer(    ModelTransform + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(4 * sizeof(float) * i));
		glVertexAttribDivisor(    ModelTransform + i, 1);
	}

	//For the other 5 layout mapped buffers as well as the index buffer, we need to get data from Assimp:

	//Model space positions:
	if (!src->HasPositions())
	{
		error("Assimp Mesh had no position vertex data!");
		return;
	}

	fillBuffer(ModelSpace, src->mVertices, src->mNumVertices * sizeof(aiVector3D), 3);

	if (src->HasNormals())
	{
		fillBuffer(NormalVector, src->mNormals, src->mNumVertices * sizeof(aiVector3D), 3);
		hasNormals = true;
	}

	if (src->HasTangentsAndBitangents())
	{
		fillBuffer(TangentVector,   src->mTangents,   src->mNumVertices * sizeof(aiVector3D), 3);
		fillBuffer(BitangentVector, src->mBitangents, src->mNumVertices * sizeof(aiVector3D), 3);
		hasTangents = true;

		if (!hasNormals)
			error("A model had tangents and bitangents but not normals somehow.");
	}

	if (src->GetNumUVChannels() == 1)
	{
		std::vector<glm::vec2> data;
		for (unsigned int b = 0; b < src->mNumVertices; b++)
			data.push_back(glm::vec2(src->mTextureCoords[0][b].x, src->mTextureCoords[0][b].y));

		fillBuffer(TextureCoords, &data[0][0], ((unsigned int)data.size()) * sizeof(glm::vec2), 2);
	}
	else if(src->GetNumUVChannels() > 1)
		error("File has more than one UV channel which is not supported!");

	if (!src->HasFaces())
	{
		error("Model does not have indicies / faces even though it does have position vertices.");
		return;
	}

	std::vector<unsigned short> indices;
	for (unsigned int b = 0; b < src->mNumFaces; b++)
	{
		if (src->mFaces[b].mNumIndices != 3)
			error("Mesh has non-triangle face, ignoring. Did you know you can specify aiProcess_Triangulate to fix this?");
		else
		{
			indices.push_back(src->mFaces[b].mIndices[0]);
			indices.push_back(src->mFaces[b].mIndices[1]);
			indices.push_back(src->mFaces[b].mIndices[2]);
		}
	}
	vertexCount = (unsigned int)indices.size();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	
	glBindVertexArray(0);

	valid = true;
}

Mesh::~Mesh()
{
	glDeleteBuffers(1, &indexBuffer);
	glDeleteBuffers(8, buffers); 

	glDeleteVertexArrays(1, &vao);
}

void Mesh::render(ShaderManager* graphics, bool useMaterials) const
{
	if(instances.size() < 1)
		return;

	if (useMaterials && material)
		material->use(graphics);

	glBindVertexArray(vao);
	glDrawElementsInstanced(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, (void*)0, instances.size());
	glBindVertexArray(0);
}

ModelInstance::ModelInstance(Model* _type)
{
	if (!_type)
		return;

	type = _type;

	//Something messed up loading the model?
	if(type->allMeshes.size() < 1)
		return;

	//Create values for each mesh and node so we can always just access them by model's mesh/node index without worring about size
	for(unsigned int a = 0 ; a<type->allMeshes.size(); a++)
	{
		MeshTransforms.push_back(glm::mat4(1.0));
		MeshFlags.push_back(0);
		MeshColors.push_back(glm::vec4(0,0,0,0));
		flagsUpdated.push_back(true);
		colorsUpdated.push_back(true);
	}

	for(unsigned int a = 0; a<type->allNodes.size(); a++)
	{
		NodeRotationFixes.push_back(glm::quat(1.0,0.0,0.0,0.0));
		UseNodeRotationFix.push_back(false);
	}

	//The offset should be the same for all meshes of a given model
	bufferOffset = type->allMeshes[0]->instances.size();
	
	//Add this instance to each of its Model's Meshes 
	for (unsigned int a = 0; a < type->allMeshes.size(); a++)
		type->allMeshes[a]->instances.push_back(this);
}

ModelInstance::~ModelInstance()
{
	if (!type)
		return;
	
	//Remove this instance from all of its Model's Meshes
	for (unsigned int a = 0; a < type->allMeshes.size(); a++)
	{
		std::vector<ModelInstance*>::iterator pos = std::find(
			type->allMeshes[a]->instances.begin(),
			type->allMeshes[a]->instances.end(),
			this
		);

		if (pos != type->allMeshes[a]->instances.end())
		{
			pos = type->allMeshes[a]->instances.erase(pos);

			//Update all other instances bufferOffsets since after this instance will need to be decremented by one
			//TODO: Maybe preallocate a certain amount of instances at a time, reallocate more if needed, and only soft delete with a hidden flag most of the time
			while(pos != type->allMeshs[a].bend())
			{
				--(*pos)->bufferOffset;
				++pos;
			}
		}
	}
}

Model::Model(std::string filePath)
{

}

Model::~Model()
{

}

/*
	Assimp does this nasty thing when importing some models
	It will add a billion superfluous nodes on top of each node
	If you had Torso -> Left Arm -> Left Hand it would give you
	Torso_RotationPivot -> Torso_Translation -> Torso->Rotation -> Torso_Scale -> LeftArm_RotationPivot...
	This just makes parsing the hierarchy less efficient and we can generally get rid of all these
*/
std::string stripSillyAssimpNodeNames(std::string in)
{
	if (in.find("_$AssimpFbx$_") != std::string::npos)
		return in.substr(0, in.find("_$AssimpFbx$_"));
	else
		return in;
}

Node::Node(aiNode const* const src, Model * parent)
{
	parent->allNodes.push_back(this);
	name = src->mName.C_Str();

	if (name.find("_RotationPivot") != std::string::npos)
	{
		glm::vec3 scale, skew, trans;
		glm::vec4 perspective;
		glm::quat rot;
		glm::mat4 to;
		CopyaiMat(src->mTransformation, to);
		glm::decompose(to, scale, rot, trans, skew, perspective);
		rotationPivot = trans;
	}

	name = stripSillyAssimpNodeNames(name);
}

void Node::foldNodeInto(aiNode const* const source, Model* parent)
{

}

Node::~Node()
{

}
