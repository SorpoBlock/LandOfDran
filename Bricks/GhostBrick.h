#pragma once

#include "Brick.h"
#include "../Interface/InputMap.h"

/*
	Client only: the translucent brick a player positions and plants, controlled like the old game's temp brick
*/
class GhostBrick
{
	Brick brick;

	bool visible = false;

	//A brick has been picked in the brick selector, so clicking the world can spawn the ghost
	bool hasSelection = false;

	//Toggled, moves by the brick's own size instead of one stud or plate
	bool superShift = false;

	//Key repeat state for each movement command, see update
	struct HeldKey
	{
		bool down = false;
		float heldMS = 0;
		float sinceRepeatMS = 0;
	};
	HeldKey heldKeys[8];

	void move(int dx, int dy, int dz);
	void rotate(int quarterTurns);

	public:

	//From the brick selector, keeps the ghost where it is if it's already out
	void select(int width, int height, int length, const glm::u8vec4& color);

	bool canSpawn() const { return hasSelection; }

	//Places the ghost just outside a surface hit by a raycast from the camera
	void spawnAt(const glm::vec3& hitPoint, const glm::vec3& hitNormal);

	bool isSuperShift() const { return superShift; }

	void hide() { visible = false; }
	bool isVisible() const { return visible; }

	const Brick& get() const { return brick; }

	//Movement is relative to the horizontal axis the camera faces most, holding a key repeats after a short delay
	void update(float deltaT, std::shared_ptr<InputMap> input, const glm::vec3& cameraDirection);
};
