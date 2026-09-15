#pragma once

#include "Brick.h"

/*
	Client only: the box a player draws around bricks to slice them into a vehicle, like the old game's selection box
	Press the select key, click where the vehicle is, drag the box's faces out around it, then plant (Enter) to slice
	Snapped to the brick grid, so it's always whole studs wide and whole plates tall
*/
class SelectionBox
{
	public:

	enum Phase
	{
		Idle,				//Not selecting
		WaitingForClick,	//The select key was pressed, the next click starts the box there
		Selecting,			//The box is out, faces can be dragged
		Stretching			//A face is being dragged while left mouse is held
	};

	//Largest the box can be dragged, in studs and plates
	static constexpr int maxStuds = 64;
	static constexpr int maxPlates = 160;

	private:

	Phase phase = Idle;

	//Grid voxels, min inclusive and max exclusive
	glm::ivec3 min = glm::ivec3(0);
	glm::ivec3 max = glm::ivec3(1);

	//0 to 5: -x, +x, -y, +y, -z, +z, -1 for none
	int hoveredFace = -1;
	int draggedFace = -1;

	//Where the dragged face was when the drag started, grid units, and how far along its axis the camera ray was then, world units
	int dragStart = 0;
	float dragAnchor = 0;
	glm::vec3 dragPivot = glm::vec3(0);

	//The face the ray from start going direction enters the box through, or leaves it through from inside, -1 if it misses
	int faceUnderRay(const glm::vec3& start, const glm::vec3& direction) const;

	//How far along the face's axis, through dragPivot, the camera ray passes closest, false if it looks straight along the axis
	bool alongAxis(int face, const glm::vec3& start, const glm::vec3& direction, float& along) const;

	public:

	Phase getPhase() const { return phase; }
	bool isActive() const { return phase != Idle; }

	//The select key: starts waiting for a click, or puts the box away if it's already out
	void toggle();
	void cancel() { phase = Idle; draggedFace = -1; hoveredFace = -1; }

	/*
		Left mouse was pressed with the camera there, hit is what the crosshair's on (hasHit false for nothing in reach)
		Returns true if the box used the click, which should then do nothing else
	*/
	bool press(const glm::vec3& cameraPosition, const glm::vec3& cameraDirection, bool hasHit, const glm::vec3& hit);

	//Left mouse was let go
	void release();

	//Each frame, drags the held face along with the crosshair and finds the face the crosshair is on
	void update(const glm::vec3& cameraPosition, const glm::vec3& cameraDirection);

	glm::ivec3 getMin() const { return min; }
	glm::ivec3 getMax() const { return max; }

	//The box as a brick for the renderer's ghost brick drawing
	Brick getBoxBrick() const;

	//A plate thin brick along the face the crosshair is on or being dragged, false for none
	bool getFaceBrick(Brick& face) const;
};
