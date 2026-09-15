#include "BrickDebris.h"

//Or the whole lifetime, if that's shorter
static constexpr float fadeMS = 400.0f;

//Enough for a few quick undos, more than this at once are simply removed without the effect
static constexpr size_t maxPieces = 64;

//Weaker than the world's gravity so the pop reads as a little hop rather than an instant drop
static const btVector3 debrisGravity = btVector3(0, -25, 0);

void BrickDebris::setLifetime(float seconds)
{
	lifetimeMS = std::max(seconds, 0.0f) * 1000.0f;

	if (lifetimeMS > 0)
		return;

	for (Piece& piece : pieces)
		destroy(piece);
	pieces.clear();
}

void BrickDebris::spawn(const Brick& brick)
{
	std::uniform_real_distribution<float> sideways(-4.5f, 4.5f);
	std::uniform_real_distribution<float> upward(9.0f, 13.5f);
	float x = sideways(random);
	float y = upward(random);
	float z = sideways(random);
	spawn(brick, btTransform(btQuaternion::getIdentity(), g2b3(brick.getWorldCenter())), btVector3(x, y, z));
}

void BrickDebris::spawn(const Brick& brick, const btTransform& placement, const btVector3& velocity)
{
	if (lifetimeMS <= 0 || pieces.size() >= maxPieces)
		return;

	Piece piece;
	piece.brick = brick;

	const SpecialBrickType* special = brick.isSpecial() && types ? types->getSpecial(brick.typeID - 1) : nullptr;
	if (special && special->shape)
	{
		piece.shape = special->shape;
		piece.ownsShape = false;
	}
	else
	{
		glm::vec3 size = glm::vec3(brick.footprintWidth() * STUD_SIZE, brick.height * PLATE_SIZE, brick.footprintLength() * STUD_SIZE);
		piece.shape = new btBoxShape(g2b3(size * 0.5f));
	}

	btScalar mass = 1;
	btVector3 inertia;
	piece.shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(mass, nullptr, piece.shape, inertia);
	info.m_startWorldTransform = placement;
	if (!piece.ownsShape)
		info.m_startWorldTransform.setRotation(placement.getRotation() * btQuaternion(btVector3(0, 1, 0), brick.getAngle()));

	piece.body = new btRigidBody(info);
	piece.body->setActivationState(DISABLE_DEACTIVATION);

	//Debris group: bumps into the world, other debris, and players, but raycasts and movement sweeps skip it, see PhysicsWorld
	world->addBody(piece.body, btBroadphaseProxy::DebrisFilter,
		btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DebrisFilter | btBroadphaseProxy::DefaultFilter);
	piece.body->setGravity(debrisGravity);

	std::uniform_real_distribution<float> spin(-5.0f, 5.0f);
	piece.body->setLinearVelocity(velocity);
	piece.body->setAngularVelocity(btVector3(spin(random), spin(random), spin(random)));

	pieces.push_back(piece);
}

void BrickDebris::destroy(Piece& piece)
{
	world->removeBody(piece.body);
	delete piece.body;
	if (piece.ownsShape)
		delete piece.shape;
}

void BrickDebris::update(float deltaT)
{
	for (Piece& piece : pieces)
		piece.ageMS += deltaT;

	auto expired = [this](Piece& piece)
	{
		if (piece.ageMS < lifetimeMS)
			return false;
		destroy(piece);
		return true;
	};

	pieces.erase(std::remove_if(pieces.begin(), pieces.end(), expired), pieces.end());
}

void BrickDebris::render(std::shared_ptr<ShaderManager> shaders, const InstancedBrickRenderer* renderer) const
{
	if (pieces.empty())
		return;

	std::vector<InstancedBrickRenderer::LooseBrick> looseBricks;
	for (const Piece& piece : pieces)
	{
		glm::mat4 transform;
		piece.body->getWorldTransform().getOpenGLMatrix(&transform[0][0]);

		float fade = std::clamp((lifetimeMS - piece.ageMS) / std::min(fadeMS, lifetimeMS), 0.0f, 1.0f);
		looseBricks.push_back({ &piece.brick, transform, piece.brick.color.a / 255.0f * fade });
	}

	renderer->renderLoose(shaders, looseBricks);
}

BrickDebris::BrickDebris(std::shared_ptr<PhysicsWorld> _world, const BrickTypes* _types) : world(_world), types(_types), random(std::random_device{}())
{
}

BrickDebris::~BrickDebris()
{
	for (Piece& piece : pieces)
		destroy(piece);
}
