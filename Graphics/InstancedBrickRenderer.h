#pragma once

#include "../LandOfDran.h"
#include "../Bricks/Brick.h"
#include "Material.h"
#include "ShaderSpecification.h"

/*
	Draws every brick as one instance of a shared cube, grouped into chunks so edits only re-upload one chunk and
	chunks outside the view are skipped
	Each face keeps its own texture mapping, so neighbouring bricks show seams like the old game
*/
class InstancedBrickRenderer
{
	struct Chunk
	{
		int64_t key = 0;
		std::vector<Brick*> bricks;
		bool dirty = false;

		//World space bounds of every brick in the chunk, as of the last rebuild
		glm::vec3 min = glm::vec3(0);
		glm::vec3 max = glm::vec3(0);

		//Index 0 is opaque bricks, 1 is transparent bricks
		GLuint vao[2] = { 0, 0 };
		GLuint instanceBuffer[2] = { 0, 0 };
		GLsizei count[2] = { 0, 0 };
	};

	//A VAO and instance buffer to draw drawInstances with
	struct InstanceSet
	{
		GLuint vao;
		GLsizei count;
	};

	std::unordered_map<int64_t, Chunk*> chunks;
	std::vector<Chunk*> dirtyChunks;

	GLuint cubeBuffer = 0;

	//One instance, for the ghost brick
	GLuint ghostVao = 0;
	GLuint ghostInstanceBuffer = 0;

	Material* topMaterial = nullptr;
	Material* bottomMaterial = nullptr;
	Material* sideMaterial = nullptr;

	GLint tileByStudsUniform = -1;

	void createInstancedVao(GLuint& vao, GLuint& instanceBuffer) const;

	Chunk* getChunk(const Brick* brick);
	void markDirty(Chunk* chunk);
	void rebuild(Chunk* chunk);
	void destroyChunk(Chunk* chunk);

	//Draws each face group with its material, for every instance set
	void drawInstances(std::shared_ptr<ShaderManager> shaders, const std::vector<InstanceSet>& sets) const;

	public:

	void addBrick(Brick* brick);
	void removeBrick(Brick* brick);

	//Call after changing a brick's color
	void updateBrick(Brick* brick);

	void clear();

	//Re-uploads chunks changed since the last call, stopping once budgetMS has been spent
	void rebuildDirty(float budgetMS);

	//Expects shaders->brickShader to be in use, culls chunks against the camera currently in shaders->cameraUniforms
	void render(std::shared_ptr<ShaderManager> shaders, bool transparent) const;

	//Expects shaders->brickShader to be in use, draws one translucent brick
	void renderGhost(std::shared_ptr<ShaderManager> shaders, const Brick& ghost) const;

	//Expects shaders->brickShadowShader to be in use, draws every opaque brick
	void renderShadows() const;

	InstancedBrickRenderer(std::shared_ptr<ShaderManager> shaders, std::shared_ptr<TextureManager> textures);
	~InstancedBrickRenderer();
};
