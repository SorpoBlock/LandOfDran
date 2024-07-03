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

	void addBody(btRigidBody* body)
	{
		world->addRigidBody(body);
	}

	void removeBody(btRigidBody* body)
	{
		world->removeRigidBody(body);
	}

	//I think it returns how many substeps were used or something? 
	int step(float deltaT)
	{
		return world->stepSimulation(deltaT / 1000.0f);
	}

	PhysicsWorld()
	{
		collisionConfig = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfig);
		broadphase = new btDbvtBroadphase();
		solver = new btSequentialImpulseConstraintSolver;
		btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);
		world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);

		//Added only after it was apparently needed for radiusImpulse, might have preformance impact?
		pairCallback = new btGhostPairCallback();
		world->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(pairCallback);
		btVector3 gravity = btVector3(0, -70, 0);
		world->setGravity(gravity);

		//Very important to call this, forgetting to do so will massivly increase the performance impact of having static objects like bricks 
		world->setForceUpdateAllAabbs(false);

		planeShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
		planeState = new btDefaultMotionState();
		btRigidBody::btRigidBodyConstructionInfo planeCon(0, planeState, planeShape);
		groundPlane = new btRigidBody(planeCon);
		groundPlane->setFriction(1.0);
		groundPlane->setUserIndex(RigidBodyUserIndex::groundPlane);
		world->addRigidBody(groundPlane);
	}

	~PhysicsWorld()
	{
		world->removeRigidBody(groundPlane);//?
		delete groundPlane;
		delete planeShape;
		delete planeState;

		delete world;
		delete solver;
		delete broadphase;
		delete dispatcher;
		delete collisionConfig;
		delete pairCallback;
	}
};

