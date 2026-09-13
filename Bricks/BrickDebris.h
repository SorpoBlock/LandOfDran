#pragma once

#include "Brick.h"
#include "../Physics/PhysicsWorld.h"
#include "../Graphics/InstancedBrickRenderer.h"

#include <random>

/*
	Client only: removed bricks that pop loose, tumble a little, and fade out, e.g. after undo
	Purely visual, the server never knows about them, and raycasts and movement sweeps ignore them
*/
class BrickDebris
{
	struct Piece
	{
		Brick brick;
		btRigidBody* body = nullptr;
		btBoxShape* shape = nullptr;
		float ageMS = 0;
	};

	std::vector<Piece> pieces;
	std::shared_ptr<PhysicsWorld> world = nullptr;
	std::mt19937 random;

	//See setLifetime
	float lifetimeMS = 3600.0f;

	void destroy(Piece& piece);

	public:

	//From graphics/brickdebrisseconds, 0 turns the effect off and clears any debris already out
	void setLifetime(float seconds);

	void spawn(const Brick& brick);

	//Removes pieces that have finished fading out
	void update(float deltaT);

	//Expects shaders->brickShader to be in use
	void render(std::shared_ptr<ShaderManager> shaders, const InstancedBrickRenderer* renderer) const;

	explicit BrickDebris(std::shared_ptr<PhysicsWorld> _world);
	~BrickDebris();
};
