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

	//See getGeneration
	unsigned int generation = 0;

	GLuint cubeBuffer = 0;

	//One instance, for the ghost brick and loose bricks
	GLuint singleVao = 0;
	GLuint singleInstanceBuffer = 0;

	Material* topMaterial = nullptr;
	Material* bottomMaterial = nullptr;
	Material* sideMaterial = nullptr;

	GLint tileByStudsUniform = -1;
	GLint brickTransformUniform = -1;
	GLint glowUniform = -1;

	void createInstancedVao(GLuint& vao, GLuint& instanceBuffer) const;

	Chunk* getChunk(const Brick* brick);
	void markDirty(Chunk* chunk);
	void rebuild(Chunk* chunk);
	void destroyChunk(Chunk* chunk);

	void setTransform(const glm::mat4& transform) const;
	void uploadSingleInstance(const glm::vec3& corner, const Brick& brick, float alpha) const;

	//Draws each face group with its material, for every instance set, calling beforeEach(set index) before each draw
	void drawInstances(std::shared_ptr<ShaderManager> shaders, const std::vector<InstanceSet>& sets, const std::function<void(size_t)>& beforeEach = nullptr) const;

	public:

	//A brick drawn on its own with any rotation, centered on the transform's origin
	struct LooseBrick
	{
		const Brick* brick;
		glm::mat4 transform;
		float alpha;
	};

	void addBrick(Brick* brick);
	void removeBrick(Brick* brick);

	//Call after changing a brick's color
	void updateBrick(Brick* brick);

	void clear();

	//Re-uploads chunks changed since the last call, stopping once budgetMS has been spent
	void rebuildDirty(float budgetMS);

	//Expects shaders->brickShader to be in use, culls chunks against the camera currently in shaders->cameraUniforms
	void render(std::shared_ptr<ShaderManager> shaders, bool transparent) const;

	//Expects shaders->brickShader to be in use, draws one translucent brick that brightens and turns more opaque as pulse goes from 0 to 1
	void renderGhost(std::shared_ptr<ShaderManager> shaders, const Brick& ghost, float pulse) const;

	/*
		Expects shaders->brickShader to be in use
		Draws blended since loose bricks fade out, but still writes depth so later transparent geometry can't show through them
	*/
	void renderLoose(std::shared_ptr<ShaderManager> shaders, const std::vector<LooseBrick>& bricks) const;

	//Expects shaders->brickShadowCascadeShader or brickShadowTintShader to be in use, draws the chunks that can cast into one shadow cascade
	void renderShadowCascade(const glm::mat4& lightSpaceMatrix, bool opaque, bool transparent) const;

	bool hasTransparentBricks() const;

	//Goes up every time a chunk is rebuilt, so anything drawn from the bricks earlier (like point light shadows) knows they've changed
	unsigned int getGeneration() const { return generation; }

	InstancedBrickRenderer(std::shared_ptr<ShaderManager> shaders, std::shared_ptr<TextureManager> textures);
	~InstancedBrickRenderer();
};
