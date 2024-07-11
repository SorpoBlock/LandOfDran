#pragma once

#include "../LandOfDran.h"

//#define BT_USE_DOUBLE_PRECISION Preferred, but will cause linking errors with currently libraries
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

//glm::vec3 to btVector3
inline btVector3 g2b3(const glm::vec3 &in)
{
	return btVector3(in.x, in.y, in.z);
}

//btVector3 to glm::vec3
inline glm::vec3 b2g3(const btVector3 &in)
{
	return glm::vec3(in.x(), in.y(), in.z());
}

/*
	Passed to btRigidBody through setUserIndex
	You can getUserIndex to figure out what type of pointer the btRigidBody's getUserData is meant to be
	This is to physics code as SimObjectType is to net code
	REMINDER: btRigidBody user data if not nullptr is always a pointer *to* a smart pointer *to* the underlying SimObject
*/
enum RigidBodyUserIndex
{
	groundPlane = 10,		//The single infinite ground plane at the bottom of the world created on start-up
	dynamicBody = 20
};

/*
	Holds a btDynamicsWorld but also a few extra things like the ground plane we'll always have
*/
class PhysicsWorld
{
	private:

	btDefaultCollisionConfiguration* collisionConfig = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* broadphase = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDiscreteDynamicsWorld* world = nullptr;
	btCollisionShape* planeShape = nullptr;
	btDefaultMotionState* planeState = nullptr;
	btRigidBody* groundPlane = nullptr;
	btGhostPairCallback* pairCallback = nullptr;

	public:

	btRigidBody *doRaycast(const btVector3 &start,const btVector3 &end,btRigidBody *ignore,btVector3 &hitPos,btVector3 &hitNormal) const;
	btRigidBody *doRaycast(const btVector3 &start,const btVector3 &end,btRigidBody *ignore) const;

	void addBody(btRigidBody* body)
	{
		world->addRigidBody(body);
	}

	void removeBody(btRigidBody* body)
	{
		world->removeRigidBody(body);
	}

	//I think it returns how many substeps were used or something? 
	int step(float deltaT);

	PhysicsWorld();

	~PhysicsWorld();
};

