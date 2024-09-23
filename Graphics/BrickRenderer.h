#pragma once

#include "../LandOfDran.h"

#include "../External/RTree.h"
#include <bit>

//Handles an entry for a single brick in the BrickRenderer
struct BrickRenderData
{
	//World position:
	short x, y, z;

	//Dimensions:
	short w, h, l;

	//From a palette:
	unsigned char brickColor = 0;

	//Which chunk this brick is in:
	unsigned short chunkId = 0;

	//Position within a chunks vector:
	unsigned short chunkIter = 0;

	void debugPrint();
};

typedef RTree<BrickRenderData*, char, 3, float> BrickTree;

//Greedy mesher where bricks are made up of voxels, a chunk is 32x32x32 bricks
class BrickChunk
{
	unsigned int chunkPosX = 0;
	unsigned int chunkPosY = 0;
	unsigned int chunkPosZ = 0;

	unsigned short chunkId = 0;

	std::vector<BrickRenderData*> bricks;
	BrickTree brickTree;

	GLuint vao;
	GLuint buffers[4];//vertex, facePos, faceSize, faceDir
	unsigned int instanceCount;

	void getFaces(int dir, std::vector<glm::vec3>& positions,std::vector<glm::vec2> &sizes,std::vector<unsigned int> &directions);

	public:

	unsigned int getNumBricks() const { return bricks.size(); }

	void addBrick(BrickRenderData* brick);
	void removeBrick(BrickRenderData* brick);

	void getFaces();

	void render(GLint brickChunkPos);

	void deleteAllBricks();

	BrickChunk(unsigned int x, unsigned int y, unsigned int z, unsigned short id) : chunkPosX(x), chunkPosY(y), chunkPosZ(z), chunkId(id) {}
};

//Holds an array of BrickChunks, handles chunk level culling, also holds textures and such for bricks
class BrickRenderer
{
	unsigned int lastChunkId = 0;
	RTree<BrickChunk*, int, 3, float> chunkTree;
	std::vector<BrickChunk*> chunks;

	public:

	void recompile();
	void render(GLint brickChunkPos);
	void addBrick(BrickRenderData* brick);
	void removeBrick(BrickRenderData* brick);
};
