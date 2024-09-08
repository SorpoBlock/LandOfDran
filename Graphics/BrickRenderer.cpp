#include "BrickRenderer.h"

void BrickRenderData::debugPrint()
{
	std::cout << "BrickRenderData\n";
	std::cout << "x: " << x << " y: " << y << " z: " << z << "\n";
	std::cout << "w: " << w << " h: " << h << " l: " << l << "\n";
	std::cout << "brickColor: " << (int)brickColor << "\n";
	std::cout << "chunkId: " << chunkId << "\n";
	std::cout << "chunkIter: " << chunkIter << "\n";
	std::cout << "\n\n";
}

void BrickChunk::deleteAllBricks()
{
	for (BrickRenderData* brick : bricks)
		delete brick;
	bricks.clear();
	brickTree.RemoveAll();
}

void BrickChunk::addBrick(BrickRenderData* brick)
{
	char min[3] = { brick->x - chunkPosX, brick->y - chunkPosY, brick->z - chunkPosZ };
	char max[3] = { (brick->x - chunkPosX) + brick->w - 1, (brick->y - chunkPosY) + brick->h - 1, (brick->z - chunkPosZ) + brick->l - 1 };
	brickTree.Insert(min, max, brick);
	brick->chunkIter = bricks.size();
	brick->chunkId = chunkId;
	bricks.push_back(brick);
}

void BrickChunk::removeBrick(BrickRenderData* brick)
{
	char min[3] = { brick->x - chunkPosX, brick->y - chunkPosY, brick->z - chunkPosZ };
	char max[3] = { (brick->x - chunkPosX) + brick->w, (brick->y - chunkPosY) + brick->h, (brick->z - chunkPosZ) + brick->l };
	brickTree.Remove(min, max, brick);
	bricks.erase(std::remove(bricks.begin(), bricks.end(), brick), bricks.end());
}

BrickRenderData *foundBrick = nullptr;

bool StopAtFirstHit(BrickRenderData* val) 
{ 
	foundBrick = val;
	return val;
}

void BrickChunk::getVerts()
{
	std::vector<glm::vec3> verts;

	for (unsigned int layer = 0; layer < 32; layer++)
	{
		unsigned int layerFaceMask[32];
		std::fill(layerFaceMask, layerFaceMask + 32, 0);

		for (unsigned int i = 0; i < 32 * 32; i++)
		{
			unsigned int row = i % 32;
			unsigned int col = (i / 32);

			//Get the brick at this position
			char min[3] = { row, col, layer };
			char max[3] = { row, col, layer };
			foundBrick = nullptr;
			brickTree.Search(min, max, StopAtFirstHit);
			BrickRenderData* brick = foundBrick;

			if (!brick)
				continue;

			/*char min[3] = {row, col, layer + 1};
			char max[3] = { row, col, layer + 1 };
			foundBrick = nullptr;
			brickTree.Search(min, max, StopAtFirstHit);
			BrickRenderData* brick = foundBrick;

			if (brick)
				continue;*/

			layerFaceMask[row] = ((1 << col) * 1) | layerFaceMask[row];
		}

		for (unsigned int row = 0; row < 32; row++)
		{
			unsigned int y = 0;
			while (y < 32)
			{
				y += std::countr_zero(layerFaceMask[row] >> y);
				if (y >= 32)
					break;

				unsigned int h = std::countr_one(layerFaceMask[row] >> y);
				unsigned int hAsMask = (h < 32U) ? ((1U << h) - 1U) : ~0U;
				unsigned int mask = hAsMask << y;

				unsigned int w = 1;
				while (row + w < 32)
				{
					unsigned int next_row_h = (layerFaceMask[row+w] >> y) & hAsMask;

					if (next_row_h != mask)
						break;

					layerFaceMask[row + w] = layerFaceMask[row + w] & ~mask;
					++w;
				}

				verts.push_back(glm::vec3(row,y,layer));
				verts.push_back(glm::vec3(row+w,y,layer));
				verts.push_back(glm::vec3(row+w,y+h,layer));
				verts.push_back(glm::vec3(row+w,y+h,layer));
				verts.push_back(glm::vec3(row,y+h,layer));
				verts.push_back(glm::vec3(row,y,layer));

				y += h;
			}
		}
	}

	vertSize = verts.size();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), &verts[0][0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);
}

void BrickChunk::render()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertSize);
	glBindVertexArray(0);
}