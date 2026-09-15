#include "SelectionBox.h"

#include <cfloat>
#include <cmath>

static const glm::vec3 gridScale = glm::vec3(STUD_SIZE, PLATE_SIZE, STUD_SIZE);

void SelectionBox::toggle()
{
	if (phase == Idle)
		phase = WaitingForClick;
	else
		cancel();
}

int SelectionBox::faceUnderRay(const glm::vec3& start, const glm::vec3& direction) const
{
	glm::vec3 low = glm::vec3(min) * gridScale;
	glm::vec3 high = glm::vec3(max) * gridScale;

	//Slabs, remembering which face each end of the overlap is on
	float enter = -FLT_MAX;
	float exit = FLT_MAX;
	int enterFace = -1;
	int exitFace = -1;

	for (int axis = 0; axis < 3; axis++)
	{
		if (std::abs(direction[axis]) < 0.000001f)
		{
			if (start[axis] < low[axis] || start[axis] > high[axis])
				return -1;
			continue;
		}

		float nearT = (low[axis] - start[axis]) / direction[axis];
		float farT = (high[axis] - start[axis]) / direction[axis];
		int nearFace = axis * 2;
		int farFace = axis * 2 + 1;
		if (nearT > farT)
		{
			std::swap(nearT, farT);
			std::swap(nearFace, farFace);
		}

		if (nearT > enter)
		{
			enter = nearT;
			enterFace = nearFace;
		}
		if (farT < exit)
		{
			exit = farT;
			exitFace = farFace;
		}
	}

	if (enter > exit || exit < 0)
		return -1;

	//Standing inside the box, the face in front of the camera is the one it looks out through
	return enter >= 0 ? enterFace : exitFace;
}

bool SelectionBox::alongAxis(int face, const glm::vec3& start, const glm::vec3& direction, float& along) const
{
	glm::vec3 axis(0);
	axis[face / 2] = 1;

	//Closest points between the camera ray and the line through the pivot along the axis
	glm::vec3 between = dragPivot - start;
	float axisDotDirection = glm::dot(axis, direction);
	float denominator = glm::dot(direction, direction) - axisDotDirection * axisDotDirection;
	if (denominator < 0.0001f)
		return false;

	along = (axisDotDirection * glm::dot(direction, between) - glm::dot(direction, direction) * glm::dot(axis, between)) / denominator;
	return true;
}

bool SelectionBox::press(const glm::vec3& cameraPosition, const glm::vec3& cameraDirection, bool hasHit, const glm::vec3& hit)
{
	if (phase == WaitingForClick)
	{
		if (!hasHit)
			return true;

		//The voxel just inside whatever was clicked, nudged along the ray so a click on a face lands in the brick behind it
		glm::vec3 inside = hit + glm::normalize(cameraDirection) * 0.01f;
		min = glm::ivec3(glm::floor(inside / gridScale));
		max = min + glm::ivec3(1);
		phase = Selecting;
		return true;
	}

	if (phase != Selecting)
		return phase != Idle;

	int face = faceUnderRay(cameraPosition, cameraDirection);
	if (face == -1)
		return true;

	int axis = face / 2;
	glm::vec3 low = glm::vec3(min) * gridScale;
	glm::vec3 high = glm::vec3(max) * gridScale;
	dragPivot = (low + high) * 0.5f;
	dragPivot[axis] = (face % 2 ? high : low)[axis];
	dragStart = face % 2 ? max[axis] : min[axis];

	float along;
	if (!alongAxis(face, cameraPosition, cameraDirection, along))
		return true;

	dragAnchor = along;
	draggedFace = face;
	phase = Stretching;
	return true;
}

void SelectionBox::release()
{
	if (phase == Stretching)
		phase = Selecting;
	draggedFace = -1;
}

void SelectionBox::update(const glm::vec3& cameraPosition, const glm::vec3& cameraDirection)
{
	if (phase == Stretching && draggedFace != -1)
	{
		int axis = draggedFace / 2;
		float along;
		if (alongAxis(draggedFace, cameraPosition, cameraDirection, along))
		{
			int limit = axis == 1 ? maxPlates : maxStuds;
			int moved = dragStart + (int)std::lround((along - dragAnchor) / gridScale[axis]);

			if (draggedFace % 2)
				max[axis] = std::clamp(moved, min[axis] + 1, min[axis] + limit);
			else
				min[axis] = std::clamp(moved, max[axis] - limit, max[axis] - 1);
		}
	}

	hoveredFace = phase == Selecting ? faceUnderRay(cameraPosition, cameraDirection) : draggedFace;
}

Brick SelectionBox::getBoxBrick() const
{
	Brick box;
	box.x = min.x;
	box.y = min.y;
	box.z = min.z;
	box.width = (unsigned char)std::clamp(max.x - min.x, 1, 255);
	box.height = (unsigned char)std::clamp(max.y - min.y, 1, 255);
	box.length = (unsigned char)std::clamp(max.z - min.z, 1, 255);
	box.color = glm::u8vec4(90, 170, 255, 255);
	return box;
}

bool SelectionBox::getFaceBrick(Brick& face) const
{
	if (hoveredFace == -1 || phase == WaitingForClick || phase == Idle)
		return false;

	face = getBoxBrick();
	face.color = glm::u8vec4(255, 230, 60, 255);

	int axis = hoveredFace / 2;
	bool high = hoveredFace % 2;
	switch (axis)
	{
		case 0:
			face.width = 1;
			face.x = high ? max.x - 1 : min.x;
			break;
		case 1:
			face.height = 1;
			face.y = high ? max.y - 1 : min.y;
			break;
		case 2:
			face.length = 1;
			face.z = high ? max.z - 1 : min.z;
			break;
	}

	return true;
}
