#include "SimObject.h"

//Update this as soon as you create a physics world
std::shared_ptr<PhysicsWorld> SimObject::world = nullptr;

/*	
	This is a virtual class with its creation handled by a factory class so there's not really anything to go here
	Constructor is private/protected
*/
SimObject::SimObject()
{

}

SimObject::~SimObject()
{

}

bool SimObject::requiresNetUpdate()// const
{
    return requiresUpdate;
}

netIDType SimObject::getID() const
{
    return netID;
}

uint32_t SimObject::getCreationTime() const
{
    return creationTime;
}

