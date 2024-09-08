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
	GLuint vertexBuffer;

	unsigned int vertSize = 0;

	public:

	void addBrick(BrickRenderData* brick);
	void removeBrick(BrickRenderData* brick);

	void getVerts();

	void render();

	void deleteAllBricks();
};

//Holds an array of BrickChunks, handles chunk level culling, also holds textures and such for bricks
class BrickRenderer
{

};
