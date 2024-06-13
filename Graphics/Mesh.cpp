#include "Mesh.h"

//Used in Model::Model to read what flags we want to load our model with from a text file
std::map<std::string, int> aiProcessMap = {
	{"CalcTangentSpace",aiProcess_CalcTangentSpace},
	{"JoinIdenticalVertices",aiProcess_JoinIdenticalVertices},
	{"MakeLeftHanded",aiProcess_MakeLeftHanded},
	{"Triangulate",aiProcess_Triangulate},
	{"RemoveComponent",aiProcess_RemoveComponent},
	{"GenNormals",aiProcess_GenNormals},
	{"GenSmoothNormals",aiProcess_GenSmoothNormals},
	{"SplitLargeMeshes",aiProcess_SplitLargeMeshes},
	{"PreTransformVertices",aiProcess_PreTransformVertices},
	{"LimitBoneWeights",aiProcess_LimitBoneWeights},
	{"ValidateDataStructure",aiProcess_ValidateDataStructure},
	{"ImproveCacheLocality",aiProcess_ImproveCacheLocality},
	{"RemoveRedundantMaterials",aiProcess_RemoveRedundantMaterials},
	{"FixInfacingNormals",aiProcess_FixInfacingNormals},
	{"SortByPType",aiProcess_SortByPType},
	{"FindDegenerates",aiProcess_FindDegenerates},
	{"FindInvalidData",aiProcess_FindInvalidData},
	{"GenUVCoords",aiProcess_GenUVCoords},
	{"TransformUVCoords",aiProcess_TransformUVCoords},
	{"FindInstances",aiProcess_FindInstances},
	{"OptimizeMeshes",aiProcess_OptimizeMeshes},
	{"OptimizeGraph",aiProcess_OptimizeGraph},
	{"FlipUVs",aiProcess_FlipUVs},
	{"FlipWindingOrder",aiProcess_FlipWindingOrder},
	{"SplitByBoneCount",aiProcess_SplitByBoneCount},
	{"Debone",aiProcess_Debone},
	{"GlobalScale",aiProcess_GlobalScale},
	{"EmbedTextures",aiProcess_EmbedTextures},
	{"ForceGenNormals",aiProcess_ForceGenNormals},
	{"DropNormals",aiProcess_DropNormals},
	{"GenBoundingBoxes",aiProcess_GenBoundingBoxes},
};

void Model::printHierarchy(Node * node,int layer) const
{
	if (!node)
		node = rootNode;

	for (unsigned int a = 0; a < layer; a++)
		std::cout << "\t";
	std::cout << node->name << "\n";

	glm::vec3 scale, skew, trans;
	glm::vec4 perspective;
	glm::quat rot;
	glm::decompose(node->defaultTransform, scale, rot, trans, skew, perspective);

	for (unsigned int a = 0; a < layer + 1; a++)
		std::cout << "\t";
	std::cout << "--Translation: " << trans.x << "," << trans.y << "," << trans.z << "\n";

	for (unsigned int a = 0; a < layer + 1; a++)
		std::cout << "\t";
	std::cout << "--Rotation: " << rot.w << ", " << rot.x << ", " << rot.y << ", " << rot.z << "\n";

	for (unsigned int a = 0; a < layer + 1; a++)
		std::cout << "\t";
	std::cout << "--Scale: " << scale.x << ", " << scale.y << ", " << scale.z << "\n";

	for (unsigned int a = 0; a < node->meshes.size(); a++)
	{
		for (unsigned int a = 0; a < layer+1; a++)
			std::cout << "\t";
		std::cout << node->meshes[a]->name << ": " << node->meshes[a]->vertexCount << " verts, index: "<<node->meshes[a]->meshIndex<<"\n";
		if (node->meshes[a]->material)
		{
			for (unsigned int a = 0; a < layer + 2; a++)
				std::cout << "\t";
			std::cout << "Material: " <<node->meshes[a]->material->getName() << "\n";
		}
	}

	for (unsigned int a = 0; a < node->children.size(); a++)
		printHierarchy(node->children[a], layer + 1);
}

void ModelInstance::setModelTransform(glm::mat4 transform)
{
	wholeModelTransform = transform;
	transformUpdated = true;
	anythingUpdated = true;
}

void ModelInstance::setNodeRotation(int nodeId, glm::quat rotation)
{
	if (nodeId < 0 || nodeId >= NodeRotationFixes.size())
	{
		scope("ModeInstance::setNodeRotation");
		error("Node index out of range.");
		return;
	}

	NodeRotationFixes[nodeId] = rotation;
	UseNodeRotationFix[nodeId] = true;
	anythingUpdated = true;
}

void ModelInstance::setFlags(int meshId, unsigned int flags)
{
	if (meshId < 0 || meshId >= MeshFlags.size())
	{
		scope("ModeInstance::setFlags");
		error("Mesh index out of range.");
		return;
	}

	MeshFlags[meshId] = flags;
	flagsUpdated[meshId] = true;
	anythingUpdated = true;
}

void ModelInstance::setColor(int meshId, glm::vec4 color)
{
	if (meshId < 0 || meshId >= MeshFlags.size())
	{
		scope("ModeInstance::setColor");
		error("Mesh index out of range.");
		return;
	}

	MeshColors[meshId] = color;
	colorsUpdated[meshId] = true;
	anythingUpdated = true;
}

void ModelInstance::update(bool debug)
{
	calculateMeshTransforms(wholeModelTransform * glm::scale(glm::mat4(1.0),type->baseScale), 0, debug ? 0 : -1);
	performMeshBufferUpdates();
}

void ModelInstance::calculateMeshTransforms(glm::mat4 currentTransform,Node * currentNode,int debugLayer)
{
	if (!currentNode)
		currentNode = type->rootNode;

	if (debugLayer != -1)
	{
		for (unsigned int a = 0; a < debugLayer; a++)
			std::cout << "\t";
		std::cout << currentNode->name << "\n";
	}

	currentTransform = currentTransform * currentNode->defaultTransform;

	for (unsigned int a = 0; a < currentNode->meshes.size(); a++)
	{
		if (currentNode->meshes[a]->nonRenderingMesh)
			continue;

		MeshTransforms[currentNode->meshes[a]->meshIndex] = currentTransform;

		if (debugLayer != -1)
		{
			for (unsigned int a = 0; a < debugLayer + 1; a++)
				std::cout << "\t";
			std::cout << currentNode->meshes[a]->name << "\n";

			glm::vec3 scale, skew, trans;
			glm::vec4 perspective;
			glm::quat rot;
			glm::decompose(currentTransform, scale, rot, trans, skew, perspective);

			for (unsigned int a = 0; a < debugLayer + 1; a++)
				std::cout << "\t";
			std::cout << "--Translation: " << trans.x << "," << trans.y << "," << trans.z << "\n";

			for (unsigned int a = 0; a < debugLayer + 1; a++)
				std::cout << "\t";
			std::cout << "--Rotation: " << rot.w << ", " << rot.x << ", " << rot.y << ", " << rot.z << "\n";

			for (unsigned int a = 0; a < debugLayer + 1; a++)
				std::cout << "\t";
			std::cout << "--Scale: " << scale.x << ", " << scale.y << ", " << scale.z << "\n";
		}
	}

	for (unsigned int a = 0; a < currentNode->children.size(); a++)
		calculateMeshTransforms(currentTransform, currentNode->children[a],debugLayer == -1 ? -1 : debugLayer + 1);
}

void ModelInstance::performMeshBufferUpdates()
{
	if (!anythingUpdated)
		return;

	//Mesh by mesh, update what has changed since last call of this function
	for (unsigned int a = 0; a < type->allMeshes.size(); a++)
	{
		if (type->allMeshes[a]->nonRenderingMesh)
			continue;

		if (transformUpdated)
		{
			/*glm::vec3 trans = getTransformFromMatrix(MeshTransforms[a]);
			std::cout << "Buffer " << type->allMeshes[a]->buffers[ModelTransform] << " offset: " << sizeof(glm::mat4) * bufferOffset << " position: " << trans.x << "," << trans.y << "," << trans.z << "\n";*/
			glBindBuffer(GL_ARRAY_BUFFER, type->allMeshes[a]->buffers[ModelTransform]);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * bufferOffset, sizeof(glm::mat4), &MeshTransforms[a][0][0]);
		}

		if (flagsUpdated[a])
		{
			//std::cout << "Flags\n";
			glBindBuffer(GL_ARRAY_BUFFER, type->allMeshes[a]->buffers[InstanceFlags]);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(int) * bufferOffset, sizeof(int), &MeshFlags[a]);

			flagsUpdated[a] = false;
		}

		if (colorsUpdated[a])
		{
			//std::cout << "Colors\n";
			glBindBuffer(GL_ARRAY_BUFFER, type->allMeshes[a]->buffers[PreColor]);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * bufferOffset, sizeof(glm::vec4), &MeshColors[a][0]);

			colorsUpdated[a] = false;
		}
	}

	transformUpdated = false;
	anythingUpdated = false;
}

void Mesh::fillBuffer(LayoutSlot slot, void* data, int size, int elements)
{
	glBindBuffer(GL_ARRAY_BUFFER, buffers[slot]);
	glEnableVertexAttribArray(slot);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glVertexAttribDivisor(slot, 0);
	glVertexAttribPointer(slot, elements, GL_FLOAT, GL_FALSE, 0, 0);
}

Mesh::Mesh(aiMesh const* const src, Model const* const parent)
{
	scope("Mesh::Mesh");

	debug("Loading mesh: " + std::string(src->mName.C_Str()));

	//If we have one material, make sure we just use that, could be result of materialoverride setting in descriptor file
	if (parent->allMaterials.size() == 1)
		material = parent->allMaterials[0];
	else if (parent->allMaterials.size() > 1)	//Just use whatever materials were baked into the model file itself
	{
		if (src->mMaterialIndex < parent->allMaterials.size())
			material = parent->allMaterials[src->mMaterialIndex];
		else
			error("Material index for mesh out of range! Wanted: " + std::to_string(src->mMaterialIndex) + " total : " + std::to_string(parent->allMaterials.size()));
	}

	name = src->mName.C_Str();

	if (lowercase(name) == "collision")
	{
		nonRenderingMesh = true;
		valid = true;
		//TODO: processCollisionMesh();
		return;
	}

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
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * InstanceBufferPageSize, (void*)0, GL_DYNAMIC_DRAW);

	//A 32bit int that represents at least a few things:
	//Color-based mouse-picking IDs when mouse-picking
	//Whether to use a decal and if so which one
	//Various special effects and stuff 
	glBindBuffer(GL_ARRAY_BUFFER, buffers[InstanceFlags]);
	glEnableVertexAttribArray(InstanceFlags);
	glVertexAttribIPointer(InstanceFlags, 1, GL_UNSIGNED_INT, 0, (void*)0);
	glVertexAttribDivisor(InstanceFlags, 1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * InstanceBufferPageSize, (void*)0, GL_DYNAMIC_DRAW);

	//In OpenGL you *can* pass a mat4 as a vertex buffer based layout binding
	//But you technically need to use 4 different layout locations to achieve it, 
	//as if each one passed one row of a 4x4 matrix
	//This is per-instance as well btw 
	glBindBuffer(GL_ARRAY_BUFFER, buffers[ModelTransform]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * InstanceBufferPageSize, (void*)0, GL_DYNAMIC_DRAW);
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(ModelTransform + i);
		glVertexAttribPointer(    ModelTransform + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(4 * sizeof(float) * i));
		glVertexAttribDivisor(    ModelTransform + i, 1);
	}

	instancesAllocated = InstanceBufferPageSize;

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

	std::cout << "Num faces: " << src->mNumFaces << "\n";

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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	
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
	if (nonRenderingMesh)
		return;

	if(instances.size() < 1)
		return;

	if (useMaterials && material)
		material->use(graphics);

	glBindVertexArray(vao);
	glDrawElementsInstanced(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, (void*)0, (GLsizei)instances.size());
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
	bufferOffset = (unsigned int)type->allMeshes[0]->instances.size();
	
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

		if (pos == type->allMeshes[a]->instances.end()) //Shouldn't happen
			continue;
		
		pos = type->allMeshes[a]->instances.erase(pos);

		//Update all other instances bufferOffsets since after this instance will need to be decremented by one
		//TODO: Maybe preallocate a certain amount of instances at a time, reallocate more if needed, and only soft delete with a hidden flag most of the time
		while(pos != type->allMeshes[a]->instances.end())
		{
			--(*pos)->bufferOffset;
			++pos;
		}
	}
}

Model::Model(std::string filePath,TextureManager * textures)
{
	scope("Model::Model");

	debug("Loading model descriptor file: " + filePath);

	std::ifstream descriptorFile(filePath.c_str());

	if (!descriptorFile.is_open())
	{
		error("Could not open file " + filePath);
		return;
	}

	/*
		Allows us to force there to be just one material
		whose channels and texture paths we manually specify with included text file
		Empty string if not doing the override
	*/
	std::string overrideMaterialPath = "";

	//What Assimp flags does the model creator wish for us to import their model with
	unsigned int desiredImporterFlags = 0;
	
	//The relative file path to the actual 3d model file
	std::string modelPath = "";

	//First string is material name from model, second string is path to material descriptor text file
	std::map<std::string, std::string> materialOverrides;

	std::string line = "";
	while (!descriptorFile.eof())
	{
		getline(descriptorFile, line);

		//Every line is just two arguments separated by a tab

		if (line.length() < 1)
			continue;

		if (line.substr(0, 1) == "#")
			continue;

		size_t firstTab = line.find("\t");

		if (firstTab == std::string::npos)
		{
			error("Malformed model import text file: " + line);
			continue;
		}

		std::string argument = line.substr(0, firstTab);
		std::string value = line.substr(firstTab + 1, line.length() - (firstTab+1)); //value needs to maintain case for flags

		//Getting a few non-importer-flags out of the way:
		if (argument == "file")
		{
			modelPath = getFolderFromPath(filePath) + value;
			continue;
		}

		//Only use one material, whose textures we manually define in an external text file:
		if (argument == "singlematerial")
		{
			overrideMaterialPath = value;

			continue;
		}

		if (argument == "material")
		{
			//Okay actually this line has 3 tabs
			size_t secondTab = value.find("\t");

			if (secondTab == std::string::npos)
			{
				error("material line needs 2 arguments");
				continue;
			}

			std::string materialName = value.substr(0, secondTab);
			std::string materialPath = value.substr(secondTab + 1, value.length() - (secondTab + 1));

			materialOverrides.insert(std::pair<std::string,std::string>(materialName, materialPath));

			continue;
		}

		auto flagSearchResult = aiProcessMap.find(argument);
		//It wasn't a valid assimp flag
		if (flagSearchResult == aiProcessMap.end())
		{
			error("Invalid process flag: " + argument);
			continue;
		}

		//To be fair an argument field for these isn't really required since they're false by default...
		if (lowercase(value) != "true")
			continue;

		//They specified an additional Assimp import flag to import their model with
		desiredImporterFlags += flagSearchResult->second;
	}

	if (modelPath.length() < 1)
	{
		error("No 'file' line specified to point to the actual model file!");
		return;
	}

	debug("Loading model " + filePath + " with flags " + std::to_string(desiredImporterFlags));

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelPath, desiredImporterFlags);

	if (!scene)
	{
		error("Problem loaded model file " + modelPath);
		error(importer.GetErrorString());
		return;
	}

	//We're using singlematerial to have one material for the whole mesh!
	if(overrideMaterialPath.length() > 0)
	{
		Material* overrideMaterial = new Material(overrideMaterialPath, textures);
		if (!overrideMaterial->isValid())
			error("Material : " + overrideMaterial->getName() + " was not valid!");
		allMaterials.push_back(overrideMaterial);
	}
	else
	{
		std::string fileName = getFileFromPath(filePath);
		std::string folder = getFolderFromPath(filePath);

		debug("Expected materials: " + std::to_string(scene->mNumMaterials));

		for (unsigned int a = 0; a < scene->mNumMaterials; a++)
		{
			aiMaterial* src = scene->mMaterials[a];

			auto matPair = materialOverrides.find(src->GetName().C_Str());
			if (matPair != materialOverrides.end())
			{
				//A specific material override was defined for this material
				debug("Using override for " + matPair->first);

				Material* overrideMaterial = new Material(folder + matPair->second, textures);
				if (!overrideMaterial->isValid())
					error("Material : " + overrideMaterial->getName() + " was not valid!");
				allMaterials.push_back(overrideMaterial);

				continue;
			}

			//Here we're going to load the textures that are embedded within the model file itself
			//Assimp categorizes its textures weird, and I wouldn't be surprised if it's not even consistant across files
			//aiTextureType_DIFFUSE   == our albedo
			//aiTextureType_NORMALS   == our normal map
			//aiTextureType_SHININESS == our roughness
			//aiTextureType_METALNESS == our metalness
			//Haven't found anything for ambient occlusion yet, but that may be because FBX files just don't use it

			int numDiffuse = src->GetTextureCount(aiTextureType_DIFFUSE);
			int numNormal = src->GetTextureCount(aiTextureType_NORMALS);
			int numRough = src->GetTextureCount(aiTextureType_SHININESS);
			int numMetal = src->GetTextureCount(aiTextureType_METALNESS);

			aiString albedo;
			aiString normal;
			aiString rough;
			aiString metal;
			
			if (numDiffuse == 1)
				src->GetTexture(aiTextureType_DIFFUSE, 0, &albedo);
			else if (numDiffuse > 1)
				error("More than one diffuse texture for material " + std::string(src->GetName().C_Str()));

			if (numNormal == 1)
				src->GetTexture(aiTextureType_NORMALS, 0, &normal);
			else if (numNormal > 1)
				error("More than one normal texture for material " + std::string(src->GetName().C_Str()));

			if (numRough == 1)
				src->GetTexture(aiTextureType_SHININESS, 0, &rough);
			else if (numRough > 1)
				error("More than one roughness texture for material " + std::string(src->GetName().C_Str()));

			if (numMetal == 1)
				src->GetTexture(aiTextureType_METALNESS, 0, &metal);
			else if (numMetal > 1)
				error("More than one metalness texture for material " + std::string(src->GetName().C_Str()));

			std::string albedoPath = albedo.length > 0 ? folder + albedo.C_Str() : "";
			std::string normalPath = normal.length > 0 ? folder + normal.C_Str() : "";
			std::string roughPath  = rough.length  > 0 ? folder + rough.C_Str()  : "";
			std::string metalPath  = metal.length  > 0 ? folder + metal.C_Str()  : "";

			if (albedoPath.length() < 1 && normalPath.length() < 1)
			{
				error("Neither albedo for normal map texture specified for material " + std::string(src->GetName().C_Str()));
				continue;
			}

			debug("Loading material " + std::string(src->GetName().C_Str()));
			Material* tmp = new Material(std::string(src->GetName().C_Str()),albedoPath,normalPath,roughPath,metalPath, "",textures);
			allMaterials.push_back(tmp);

			if (!tmp->isValid())
				error("Material : " + tmp->getName() + " was not valid!");
		}
	}

	debug(std::to_string(allMaterials.size()) + " materials loaded");

	for (unsigned int a = 0; a < scene->mNumMeshes; a++)
	{
		aiMesh* src = scene->mMeshes[a];
		Mesh* tmp = new Mesh(src, this);

		tmp->meshIndex = allMeshes.size();
		allMeshes.push_back(tmp);
	}

	rootNode = new Node(scene->mRootNode, this);
}

void Mesh::recompileInstances()
{
	if (nonRenderingMesh)
		return;

	std::vector<glm::mat4> transforms;
	std::vector<unsigned int> flags;
	std::vector<glm::vec4> colors;

	for (unsigned int a = 0; a < instances.size(); a++)
	{
		transforms.push_back(instances[a]->MeshTransforms[meshIndex]);
		flags.push_back(instances[a]->MeshFlags[meshIndex]);
		colors.push_back(instances[a]->MeshColors[meshIndex]);
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffers[ModelTransform]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * transforms.size(), &transforms[0][0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[InstanceFlags]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(int) * flags.size(), &flags[0]);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[PreColor]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * colors.size(), &colors[0][0]);
}

void Model::recompileAll()
{
	for (unsigned int a = 0; a < allMeshes.size(); a++)
		allMeshes[a]->recompileInstances();
}

void Model::render(ShaderManager* graphics,bool useMaterials) const
{
	for (unsigned int a = 0; a < allMeshes.size(); a++)
		allMeshes[a]->render(graphics, useMaterials);
}

Model::~Model()
{
	//Deletion is recursive, trying to do it through allNodes would cause a crash
	if(rootNode)
		delete rootNode;

	for (unsigned int a = 0; a < allMeshes.size(); a++)
		delete allMeshes[a];

	for (unsigned int a = 0; a < allMaterials.size(); a++)
		delete allMaterials[a]; //Calls markForCleanup on associated textures
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
		glm::mat4 to;
		CopyaiMat(src->mTransformation, to);
		rotationPivot = getTransformFromMatrix(to);
	}

	name = stripSillyAssimpNodeNames(name);

	//What is this node's default state before any animations
	CopyaiMat(src->mTransformation, defaultTransform);

	//Assimp gives us meshes as indicies to an array of meshes loaded earlier
	for (unsigned int a = 0; a < src->mNumMeshes; a++)
		meshes.push_back(parent->allMeshes[src->mMeshes[a]]);

	//Recursivly collapse the bloated node hierarchy Assimp gives us
	for (unsigned int a = 0; a < src->mNumChildren; a++)
	{
		//Is a child node one of the nodes Assimp split a single original node into during importing
		if (stripSillyAssimpNodeNames(src->mChildren[a]->mName.C_Str()) == name)
			foldNodeInto(src->mChildren[a], parent);					//Collapse it into this node
		else
			children.push_back(new Node(src->mChildren[a], parent));	//Allow it to be its own node
	}
}

void Node::foldNodeInto(aiNode const* const src, Model* parent)
{
	//Just in case the rotation pivot is not the first of the node's assimp separates an origional node into
	std::string tmpname = src->mName.C_Str();
	if (tmpname.find("_RotationPivot") != std::string::npos)
	{
		glm::mat4 to;
		CopyaiMat(src->mTransformation, to);
		rotationPivot = getTransformFromMatrix(to);
	}

	//If assimp splits a single origonal node into many smaller nodes for each part of the model matrix the org. node had
	//*Probably* the last one will have all of the meshes for the original node
	//Make sure every mesh that was assigned to the original node is assigned to our reconstructed node
	for (unsigned int a = 0; a < src->mNumMeshes; a++)
	{
		//Do we already have this mesh in our node?
		std::vector<Mesh*>::iterator pos = std::find(
			meshes.begin(), 
			meshes.end(), 
			parent->allMeshes[src->mMeshes[a]]);

		//If not, add it
		if (pos == meshes.end())
			meshes.push_back(parent->allMeshes[src->mMeshes[a]]);
	}

	//Continue the folding process recursivly 
	for (unsigned int a = 0; a < src->mNumChildren; a++)
	{
		//Is a child node one of the nodes Assimp split a single original node into during importing
		if (stripSillyAssimpNodeNames(src->mChildren[a]->mName.C_Str()) == name)
			foldNodeInto(src->mChildren[a], parent);					//Collapse it into this node
		else
			children.push_back(new Node(src->mChildren[a], parent));	//Allow it to be its own node
	}
}

Node::~Node()
{
	//Recursive!
	for (unsigned int a = 0; a < children.size(); a++)
		delete children[a];
}
