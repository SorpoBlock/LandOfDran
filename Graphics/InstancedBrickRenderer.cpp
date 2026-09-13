#include "InstancedBrickRenderer.h"

#include "../Utility/GlobalStartup.h"

#include <array>
#include <cfloat>

//Position, normal, tangent, bitangent, uv
static constexpr int cubeVertexFloats = 14;
//Min corner, size, color
static constexpr int instanceFloats = 10;
//In studs horizontally and plates vertically
static constexpr int chunkSize = 64;

//Where each face group sits in the shared cube, see makeCube
static constexpr GLint topFirst = 0;
static constexpr GLsizei topCount = 6;
static constexpr GLint bottomFirst = 6;
static constexpr GLsizei bottomCount = 6;
static constexpr GLint sidesFirst = 12;
static constexpr GLsizei sidesCount = 24;

static const glm::vec3 gridScale = glm::vec3(STUD_SIZE, PLATE_SIZE, STUD_SIZE);

static int floorDiv(int value, int divisor)
{
	return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}

//A brick belongs to the chunk holding its min corner, it never moves so that never changes
static int64_t chunkKey(const Brick* brick)
{
	int64_t x = floorDiv(brick->x, chunkSize) & 0x1FFFFF;
	int64_t y = floorDiv(brick->y, chunkSize) & 0x1FFFFF;
	int64_t z = floorDiv(brick->z, chunkSize) & 0x1FFFFF;
	return (x << 42) | (y << 21) | z;
}

static void appendInstance(std::vector<float>& instances, const Brick& brick, float alpha)
{
	glm::vec3 color = glm::vec3(brick.color) / 255.0f;
	instances.insert(instances.end(), { (float)brick.x, (float)brick.y, (float)brick.z,
		(float)brick.footprintWidth(), (float)brick.height, (float)brick.footprintLength(), color.r, color.g, color.b, alpha });
}

static std::vector<float> makeCube()
{
	struct Face
	{
		glm::vec3 origin;
		glm::vec3 u;
		glm::vec3 v;
	};

	//Top, bottom, then the 4 sides, matching the draw ranges above
	//u cross v points outward, so every face winds counter-clockwise, and u/v double as tangent/bitangent
	const Face faces[6] =
	{
		{ glm::vec3(0, 1, 1), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1) },
		{ glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1) },
		{ glm::vec3(1, 0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0) },
		{ glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0) },
		{ glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) },
		{ glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0) }
	};

	const glm::vec2 corners[6] = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 0), glm::vec2(1, 1), glm::vec2(0, 1) };

	std::vector<float> data;
	for (const Face& face : faces)
	{
		glm::vec3 normal = glm::cross(face.u, face.v);
		for (const glm::vec2& uv : corners)
		{
			glm::vec3 position = face.origin + face.u * uv.x + face.v * uv.y;
			data.insert(data.end(), { position.x, position.y, position.z, normal.x, normal.y, normal.z,
				face.u.x, face.u.y, face.u.z, face.v.x, face.v.y, face.v.z, uv.x, uv.y });
		}
	}

	return data;
}

//Gribb-Hartmann: each plane is a sum or difference of the matrix's last row with one of the others
static std::array<glm::vec4, 6> frustumPlanes(const glm::mat4& viewProjection)
{
	glm::vec4 rows[4];
	for (int row = 0; row < 4; row++)
		rows[row] = glm::vec4(viewProjection[0][row], viewProjection[1][row], viewProjection[2][row], viewProjection[3][row]);

	return { rows[3] + rows[0], rows[3] - rows[0], rows[3] + rows[1], rows[3] - rows[1], rows[3] + rows[2], rows[3] - rows[2] };
}

static bool boxVisible(const std::array<glm::vec4, 6>& planes, const glm::vec3& min, const glm::vec3& max)
{
	for (const glm::vec4& plane : planes)
	{
		glm::vec3 farthest(plane.x > 0 ? max.x : min.x, plane.y > 0 ? max.y : min.y, plane.z > 0 ? max.z : min.z);
		if (glm::dot(glm::vec3(plane), farthest) + plane.w < 0)
			return false;
	}
	return true;
}

void InstancedBrickRenderer::createInstancedVao(GLuint& vao, GLuint& instanceBuffer) const
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &instanceBuffer);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, cubeBuffer);
	const int cubeAttributeSizes[5] = { 3, 3, 3, 3, 2 };
	int offset = 0;
	for (int attribute = 0; attribute < 5; attribute++)
	{
		glEnableVertexAttribArray(attribute);
		glVertexAttribPointer(attribute, cubeAttributeSizes[attribute], GL_FLOAT, GL_FALSE, cubeVertexFloats * sizeof(float), (void*)(offset * sizeof(float)));
		offset += cubeAttributeSizes[attribute];
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	const int instanceAttributeSizes[3] = { 3, 3, 4 };
	offset = 0;
	for (int a = 0; a < 3; a++)
	{
		GLuint attribute = 5 + a;
		glEnableVertexAttribArray(attribute);
		glVertexAttribPointer(attribute, instanceAttributeSizes[a], GL_FLOAT, GL_FALSE, instanceFloats * sizeof(float), (void*)(offset * sizeof(float)));
		glVertexAttribDivisor(attribute, 1);
		offset += instanceAttributeSizes[a];
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

InstancedBrickRenderer::Chunk* InstancedBrickRenderer::getChunk(const Brick* brick)
{
	int64_t key = chunkKey(brick);

	Chunk*& chunk = chunks[key];
	if (chunk)
		return chunk;

	chunk = new Chunk;
	chunk->key = key;

	for (int transparency = 0; transparency < 2; transparency++)
		createInstancedVao(chunk->vao[transparency], chunk->instanceBuffer[transparency]);

	return chunk;
}

void InstancedBrickRenderer::markDirty(Chunk* chunk)
{
	if (chunk->dirty)
		return;

	chunk->dirty = true;
	dirtyChunks.push_back(chunk);
}

void InstancedBrickRenderer::addBrick(Brick* brick)
{
	Chunk* chunk = getChunk(brick);
	brick->renderIndex = chunk->bricks.size();
	chunk->bricks.push_back(brick);
	markDirty(chunk);
}

void InstancedBrickRenderer::removeBrick(Brick* brick)
{
	auto it = chunks.find(chunkKey(brick));
	if (it == chunks.end())
		return;

	Chunk* chunk = it->second;
	Brick* last = chunk->bricks.back();
	chunk->bricks[brick->renderIndex] = last;
	last->renderIndex = brick->renderIndex;
	chunk->bricks.pop_back();
	markDirty(chunk);
}

void InstancedBrickRenderer::updateBrick(Brick* brick)
{
	auto it = chunks.find(chunkKey(brick));
	if (it != chunks.end())
		markDirty(it->second);
}

void InstancedBrickRenderer::rebuild(Chunk* chunk)
{
	if (chunk->bricks.empty())
	{
		chunks.erase(chunk->key);
		destroyChunk(chunk);
		return;
	}

	std::vector<float> instances[2];
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);

	for (const Brick* brick : chunk->bricks)
	{
		appendInstance(instances[brick->color.a < 255 ? 1 : 0], *brick, brick->color.a / 255.0f);

		glm::vec3 corner = glm::vec3(brick->x, brick->y, brick->z);
		min = glm::min(min, corner);
		max = glm::max(max, corner + glm::vec3(brick->footprintWidth(), brick->height, brick->footprintLength()));
	}

	chunk->min = min * gridScale;
	chunk->max = max * gridScale;

	for (int transparency = 0; transparency < 2; transparency++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, chunk->instanceBuffer[transparency]);
		glBufferData(GL_ARRAY_BUFFER, instances[transparency].size() * sizeof(float), instances[transparency].data(), GL_DYNAMIC_DRAW);
		chunk->count[transparency] = instances[transparency].size() / instanceFloats;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InstancedBrickRenderer::rebuildDirty(float budgetMS)
{
	float start = getTicksMS();

	while (!dirtyChunks.empty())
	{
		Chunk* chunk = dirtyChunks.back();
		dirtyChunks.pop_back();
		chunk->dirty = false;
		rebuild(chunk);

		if (getTicksMS() - start > budgetMS)
			break;
	}
}

void InstancedBrickRenderer::drawInstances(std::shared_ptr<ShaderManager> shaders, const std::vector<InstanceSet>& sets) const
{
	struct FaceGroup
	{
		const Material* material;
		GLint first;
		GLsizei count;
		bool tileByStuds;
	};

	const FaceGroup groups[3] =
	{
		{ topMaterial, topFirst, topCount, true },
		{ bottomMaterial, bottomFirst, bottomCount, true },
		{ sideMaterial, sidesFirst, sidesCount, false }
	};

	for (const FaceGroup& group : groups)
	{
		group.material->use(shaders);
		glUniform1i(tileByStudsUniform, group.tileByStuds);

		for (const InstanceSet& set : sets)
		{
			glBindVertexArray(set.vao);
			glDrawArraysInstanced(GL_TRIANGLES, group.first, group.count, set.count);
		}
	}

	glBindVertexArray(0);
}

void InstancedBrickRenderer::render(std::shared_ptr<ShaderManager> shaders, bool transparent) const
{
	int transparency = transparent ? 1 : 0;

	std::array<glm::vec4, 6> planes = frustumPlanes(shaders->cameraUniforms.CameraProjection * shaders->cameraUniforms.CameraView);

	std::vector<InstanceSet> visible;
	for (const auto& entry : chunks)
	{
		const Chunk* chunk = entry.second;
		if (chunk->count[transparency] > 0 && boxVisible(planes, chunk->min, chunk->max))
			visible.push_back({ chunk->vao[transparency], chunk->count[transparency] });
	}

	if (visible.empty())
		return;

	if (transparent)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
	}

	drawInstances(shaders, visible);

	if (transparent)
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

void InstancedBrickRenderer::renderGhost(std::shared_ptr<ShaderManager> shaders, const Brick& ghost) const
{
	std::vector<float> instance;
	appendInstance(instance, ghost, 0.5f);

	glBindBuffer(GL_ARRAY_BUFFER, ghostInstanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, instance.size() * sizeof(float), instance.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	drawInstances(shaders, { { ghostVao, 1 } });

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void InstancedBrickRenderer::renderShadows() const
{
	for (const auto& entry : chunks)
	{
		const Chunk* chunk = entry.second;
		if (chunk->count[0] < 1)
			continue;

		glBindVertexArray(chunk->vao[0]);
		glDrawArraysInstanced(GL_TRIANGLES, 0, topCount + bottomCount + sidesCount, chunk->count[0]);
	}

	glBindVertexArray(0);
}

void InstancedBrickRenderer::destroyChunk(Chunk* chunk)
{
	glDeleteVertexArrays(2, chunk->vao);
	glDeleteBuffers(2, chunk->instanceBuffer);
	delete chunk;
}

void InstancedBrickRenderer::clear()
{
	for (auto& entry : chunks)
		destroyChunk(entry.second);

	chunks.clear();
	dirtyChunks.clear();
}

InstancedBrickRenderer::InstancedBrickRenderer(std::shared_ptr<ShaderManager> shaders, std::shared_ptr<TextureManager> textures)
{
	std::vector<float> cube = makeCube();

	glGenBuffers(1, &cubeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, cubeBuffer);
	glBufferData(GL_ARRAY_BUFFER, cube.size() * sizeof(float), cube.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	createInstancedVao(ghostVao, ghostInstanceBuffer);

	topMaterial = new Material("Assets/brick/studs.txt", textures);
	bottomMaterial = new Material("Assets/brick/bottoms.txt", textures);
	sideMaterial = new Material("Assets/brick/sides.txt", textures);

	if (!topMaterial->isValid() || !bottomMaterial->isValid() || !sideMaterial->isValid())
		error("Could not load brick materials from Assets/brick/");

	tileByStudsUniform = shaders->brickShader->getUniformLocation("tileByStuds");
}

InstancedBrickRenderer::~InstancedBrickRenderer()
{
	clear();

	glDeleteVertexArrays(1, &ghostVao);
	glDeleteBuffers(1, &ghostInstanceBuffer);
	glDeleteBuffers(1, &cubeBuffer);

	delete topMaterial;
	delete bottomMaterial;
	delete sideMaterial;
}
