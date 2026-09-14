#include "GhostBrick.h"

static constexpr float repeatDelayMS = 500.0f;
static constexpr float repeatIntervalMS = 80.0f;

void GhostBrick::select(int width, int height, int length, uint16_t typeID)
{
	brick.width = width;
	brick.height = height;
	brick.length = length;
	brick.typeID = typeID;

	//Like the old game, only basic bricks resize
	if (brick.isSpecial())
		resizeMode = false;
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
	hasPosition = true;
	visible = true;
}

bool GhostBrick::show()
{
	if (!hasPosition)
		return false;

	visible = true;
	return true;
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

void GhostBrick::resizeHorizontal(int axis, int sign, int amount)
{
	//The footprint's x size is the brick's width unless it's turned a quarter
	unsigned char& size = (axis == 0) == (brick.angleID % 2 == 0) ? brick.width : brick.length;

	int newSize = std::clamp((int)size + amount, 1, 255);
	int change = newSize - (int)size;
	size = (unsigned char)newSize;

	//The min corner only moves when the face being pushed or pulled is on the negative side
	if (sign < 0)
	{
		if (axis == 0)
			brick.x -= change;
		else
			brick.z -= change;
	}
}

void GhostBrick::update(float deltaT, std::shared_ptr<InputMap> input, const glm::vec3& cameraDirection)
{
	//Polled even while hidden so a press made then doesn't fire the moment the ghost appears
	bool rotateForward = input->pollCommand(BrickRotate);
	bool rotateBack = input->pollCommand(BrickRotateBack);

	if (input->pollCommand(BrickSuperShift))
		superShift = !superShift;

	bool toggleResize = input->pollCommand(ResizeToggle);

	moved = false;
	rotated = false;

	if (!visible)
		return;

	if (toggleResize && !brick.isSpecial())
		resizeMode = !resizeMode;

	if (rotateForward)
		rotate(1);
	if (rotateBack)
		rotate(3);
	rotated = rotateForward || rotateBack;

	glm::ivec3 forward = std::abs(cameraDirection.x) > std::abs(cameraDirection.z) ?
		glm::ivec3(cameraDirection.x > 0 ? 1 : -1, 0, 0) :
		glm::ivec3(0, 0, cameraDirection.z > 0 ? 1 : -1);
	glm::ivec3 right = glm::ivec3(-forward.z, 0, forward.x);

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

		moved = true;

		if (a < 4)
		{
			if (resizeMode)
			{
				//Forward and backward work on the face ahead of the camera, right and left on the face to its right
				glm::ivec3 face = a < 2 ? forward : right;
				int amount = (a == 0 || a == 3) ? 1 : -1;
				resizeHorizontal(face.x != 0 ? 0 : 2, face.x + face.z, amount);
			}
			else
			{
				glm::ivec3 direction = a == 0 ? forward : a == 1 ? -forward : a == 2 ? -right : right;
				int stepX = superShift ? brick.footprintWidth() : 1;
				int stepZ = superShift ? brick.footprintLength() : 1;
				move(direction.x * stepX, 0, direction.z * stepZ);
			}
		}
		else
		{
			int plates = a < 6 ? 1 : 3;
			int sign = (a % 2 == 0) ? 1 : -1;

			//Resizing keeps the bottom where it is
			if (resizeMode)
				brick.height = std::clamp(brick.height + sign * plates, 1, 255);
			else
				move(0, sign * (superShift ? brick.height : plates), 0);
		}
	}
}
