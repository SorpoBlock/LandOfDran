#include "GhostBrick.h"

static constexpr float repeatDelayMS = 500.0f;
static constexpr float repeatIntervalMS = 80.0f;

void GhostBrick::select(int width, int height, int length, const glm::u8vec4& color)
{
	brick.width = width;
	brick.height = height;
	brick.length = length;
	brick.color = color;
	hasSelection = true;
}

void GhostBrick::spawnAt(const glm::vec3& hitPoint, const glm::vec3& hitNormal)
{
	//The grid cell just outside the surface that was hit
	glm::vec3 outside = hitPoint + hitNormal * 0.05f;
	int cellX = (int)floor(outside.x / STUD_SIZE);
	int cellY = (int)floor(outside.y / PLATE_SIZE);
	int cellZ = (int)floor(outside.z / STUD_SIZE);

	int footprintWidth = brick.footprintWidth();
	int footprintLength = brick.footprintLength();

	//Centered on that cell, then pushed off whichever face was hit so the whole brick is outside it
	brick.x = cellX - footprintWidth / 2;
	brick.y = cellY;
	brick.z = cellZ - footprintLength / 2;

	if (hitNormal.x > 0.5f)
		brick.x = cellX;
	else if (hitNormal.x < -0.5f)
		brick.x = cellX - footprintWidth + 1;

	if (hitNormal.z > 0.5f)
		brick.z = cellZ;
	else if (hitNormal.z < -0.5f)
		brick.z = cellZ - footprintLength + 1;

	if (hitNormal.y < -0.5f)
		brick.y = cellY - brick.height + 1;

	brick.y = std::max(brick.y, 0);
	visible = true;
}

void GhostBrick::move(int dx, int dy, int dz)
{
	brick.x += dx;
	brick.y = std::max(brick.y + dy, 0);
	brick.z += dz;
}

void GhostBrick::rotate(int quarterTurns)
{
	double centerX = brick.x + brick.footprintWidth() * 0.5;
	double centerZ = brick.z + brick.footprintLength() * 0.5;

	brick.angleID = (brick.angleID + quarterTurns) % 4;

	brick.x = (int)floor(centerX - brick.footprintWidth() * 0.5 + 0.5);
	brick.z = (int)floor(centerZ - brick.footprintLength() * 0.5 + 0.5);
}

void GhostBrick::update(float deltaT, std::shared_ptr<InputMap> input, const glm::vec3& cameraDirection)
{
	//Polled even while hidden so a press made then doesn't fire the moment the ghost appears
	bool rotateForward = input->pollCommand(BrickRotate);
	bool rotateBack = input->pollCommand(BrickRotateBack);

	if (!visible)
		return;

	if (rotateForward)
		rotate(1);
	if (rotateBack)
		rotate(3);

	glm::ivec3 forward = std::abs(cameraDirection.x) > std::abs(cameraDirection.z) ?
		glm::ivec3(cameraDirection.x > 0 ? 1 : -1, 0, 0) :
		glm::ivec3(0, 0, cameraDirection.z > 0 ? 1 : -1);
	glm::ivec3 right = glm::ivec3(-forward.z, 0, forward.x);

	//Super shift moves by the brick's own size instead of a single stud or plate
	bool superShift = input->isCommandKeydown(BrickSuperShift);

	const InputCommand commands[8] = { BrickForward, BrickBackward, BrickLeft, BrickRight, BrickUp, BrickDown, BrickUpThree, BrickDownThree };

	for (int a = 0; a < 8; a++)
	{
		HeldKey& key = heldKeys[a];
		bool down = input->isCommandKeydown(commands[a]);
		bool shouldMove = false;

		if (down && !key.down)
		{
			shouldMove = true;
			key.heldMS = 0;
			key.sinceRepeatMS = 0;
		}
		else if (down)
		{
			key.heldMS += deltaT;
			key.sinceRepeatMS += deltaT;
			if (key.heldMS > repeatDelayMS && key.sinceRepeatMS > repeatIntervalMS)
			{
				shouldMove = true;
				key.sinceRepeatMS = 0;
			}
		}

		key.down = down;

		if (!shouldMove)
			continue;

		if (a < 4)
		{
			glm::ivec3 direction = a == 0 ? forward : a == 1 ? -forward : a == 2 ? -right : right;
			int stepX = superShift ? brick.footprintWidth() : 1;
			int stepZ = superShift ? brick.footprintLength() : 1;
			move(direction.x * stepX, 0, direction.z * stepZ);
		}
		else
		{
			int plates = superShift ? brick.height : (a < 6 ? 1 : 3);
			move(0, (a % 2 == 0) ? plates : -plates, 0);
		}
	}
}
