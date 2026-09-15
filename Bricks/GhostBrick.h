#pragma once

#include "Brick.h"
#include "../Interface/InputMap.h"

/*
	Client only: the translucent brick a player positions and plants, controlled like the old game's temp brick
	Shown while a filled hot bar slot is picked, see BrickHotbar
*/
class GhostBrick
{
	Brick brick;

	bool visible = false;

	//Placed somewhere at least once, so it can come back there when building mode starts again
	bool hasPosition = false;

	//Toggled, moves by the brick's own size instead of one stud or plate
	bool superShift = false;

	//Toggled, the movement keys change the brick's size instead of its position
	bool resizeMode = false;

	//Key repeat state for each movement command, see update
	struct HeldKey
	{
		bool down = false;
		float heldMS = 0;
		float sinceRepeatMS = 0;
	};
	HeldKey heldKeys[8];

	//What the last update did, so the client can click like the old game
	bool moved = false;
	bool rotated = false;

	void move(int dx, int dy, int dz);
	void rotate(int quarterTurns);

	//Grows (amount > 0) or shrinks the footprint along x (axis 0) or z (axis 2), moving only the face on the side of sign
	void resizeHorizontal(int axis, int sign, int amount);

	public:

	//Keeps the ghost where it is if it's already out, special bricks (typeID above 0) pass their type's size and can't be resized
	void select(int width, int height, int length, uint16_t typeID = 0);

	void setColor(const glm::u8vec4& color) { brick.color = color; }
	void setMaterial(unsigned char material) { brick.material = material; }

	//Places the ghost just outside a surface hit by a raycast from the camera
	void spawnAt(const glm::vec3& hitPoint, const glm::vec3& hitNormal);

	//Shows the ghost where it last was, false if it's never been placed
	bool show();

	bool isSuperShift() const { return superShift; }

	bool isResizeMode() const { return resizeMode; }
	void setResizeMode(bool on) { resizeMode = on; }

	void hide() { visible = false; }
	bool isVisible() const { return visible; }

	const Brick& get() const { return brick; }

	//Whether the last update moved or resized the brick, or turned it
	bool didMove() const { return moved; }
	bool didRotate() const { return rotated; }

	/*
		Movement is relative to the horizontal axis the camera faces most, holding a key repeats after a short delay
		In resize mode forward, right, and up push that face of the brick out, and backward, left, and down pull it back in
	*/
	void update(float deltaT, std::shared_ptr<InputMap> input, const glm::vec3& cameraDirection);
};
