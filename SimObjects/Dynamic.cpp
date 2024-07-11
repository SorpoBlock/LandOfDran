#include "Dynamic.h"
#include "Dynamic.h"

Dynamic::Dynamic(std::shared_ptr<DynamicType> _type, const btVector3& initialPos) : type(_type)
{
	body = type->createBody();
	world->addBody(body);

	btTransform t;
	t.setIdentity();
	t.setOrigin(initialPos);
	body->setWorldTransform(t);

	if (!type->getModel()->isServerSide())
	{
		//TODO: Pass initial rotation as well
		interpolator.addSnapshot(b2g3(initialPos), glm::quat(1, 0, 0, 0), 4);
		modelInstance = new ModelInstance(type->getModel().get());
		modelInstance->setModelTransform(glm::translate(glm::vec3(initialPos.x(), initialPos.y(), initialPos.z())));
	}
}

void Dynamic::onCreation()
{
	//Pointer to a smart pointer to this
	body->setUserPointer((void*)new std::shared_ptr<SimObject>(getMe()));
}

/*
	Note, does not include packet header or anything, only the marginal bytes added by* this* object in a bigger packet
	Same goes for all packet related functions here
*/
unsigned int Dynamic::getCreationPacketBytes() const
{
	/*
		4 bytes - net ID
		4 bytes - dyanamic type ID
		position
		rotation
		scale
	*/
	return PositionBytes + QuaternionBytes + sizeof(netIDType) * 2;
}

unsigned int Dynamic::getUpdatePacketBytes() const
{
	/*
		4 bytes - net ID
		position
		rotation
	*/
	return PositionBytes*3 + QuaternionBytes + sizeof(netIDType);
}

void Dynamic::updateSnapshot()
{
	if (clientControlled)
	{
		const btTransform& t = body->getWorldTransform();
		const btVector3& v = t.getOrigin();
		const btQuaternion &q = t.getRotation();
		modelInstance->setModelTransform(glm::translate(glm::vec3(v.x(), v.y(), v.z())) * glm::toMat4(glm::quat(q.w(), q.x(), q.y(), q.z())));
	}
	else
	{
		const glm::vec3& pos = interpolator.getPosition();
		const glm::quat &quat = interpolator.getRotation();
		modelInstance->setModelTransform(glm::translate(pos) * glm::toMat4(quat));
	}
}

void Dynamic::setPosition(const btVector3& pos)
{
	btTransform t = body->getWorldTransform();
	t.setOrigin(pos);
	body->setWorldTransform(t);
}

void Dynamic::activate() const
{
	body->activate();
}

void Dynamic::setVelocity(const btVector3& vel)
{
	body->setLinearVelocity(vel);
}

void Dynamic::setAngularVelocity(const btVector3& vel)
{
	body->setAngularVelocity(vel);
}

btVector3 Dynamic::getVelocity() const
{
	return body->getLinearVelocity();
}

btVector3 Dynamic::getAngularVelocity() const
{
	return body->getAngularVelocity();
}

btVector3 Dynamic::getPosition() const
{
	return body->getWorldTransform().getOrigin();
}

bool Dynamic::requiresNetUpdate() const
{
	//For dynamics requiresUpdate means a change to something like a decal, or a node color
	if (requiresUpdate)
		return true;

	const btTransform& t = body->getWorldTransform();

	//If the object has moved more than 0.14 studs
	if (t.getOrigin().distance2(lastSentTransform.getOrigin()) > 0.005)
		return true;

	//More than like 8 degrees difference in rotation?
	if (body->getAngularVelocity().length2() > 0.1)
		return true;

	//Even if the object isn't moving much at all we should still send out an update every once in a while
	/*if (SDL_GetTicks() - lastSentTime > 500)
	{
		std::cout << "Time\n";
		return true;
	}
	This causes crashes cause the time can increment between the time we check and the time we send the packet
	*/

	return false;
}

void Dynamic::addToCreationPacket(enet_uint8 * dest) const
{
	memcpy(dest, &netID, sizeof(netIDType));

	const netIDType typeID = type->getID();
	memcpy(dest+sizeof(netIDType), &typeID, sizeof(netIDType));

	const btTransform &t = body->getWorldTransform();
	const glm::vec3 &pos = glm::vec3(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z());
	const glm::quat &quat = glm::quat(t.getRotation().w(), t.getRotation().x(), t.getRotation().y(), t.getRotation().z());

	addPosition(dest + sizeof(netIDType) * 2, pos);
	addQuaternion(dest + sizeof(netIDType) * 2 + PositionBytes, quat);
}

void Dynamic::addToUpdatePacket(enet_uint8 * dest)
{
	requiresUpdate = false;

	lastSentTime = SDL_GetTicks();
	lastSentTransform = body->getWorldTransform();

	const glm::vec3 &pos = glm::vec3(lastSentTransform.getOrigin().x(), lastSentTransform.getOrigin().y(), lastSentTransform.getOrigin().z());
	const glm::quat &quat = glm::quat(lastSentTransform.getRotation().w(), lastSentTransform.getRotation().x(), lastSentTransform.getRotation().y(), lastSentTransform.getRotation().z());
	const glm::vec3 &linVel = glm::vec3(body->getLinearVelocity().x(), body->getLinearVelocity().y(), body->getLinearVelocity().z());
	const glm::vec3 &angVel = glm::vec3(body->getAngularVelocity().x(), body->getAngularVelocity().y(), body->getAngularVelocity().z());

	memcpy(dest, &netID, sizeof(netIDType));
	addPosition(dest +   sizeof(netIDType), pos);
	addQuaternion(dest + sizeof(netIDType) + PositionBytes, quat);
	addPosition(dest +   sizeof(netIDType) + PositionBytes + QuaternionBytes, linVel);
	addPosition(dest +   sizeof(netIDType) + PositionBytes + QuaternionBytes + PositionBytes, angVel);
}

void Dynamic::requestDestruction()
{
	((std::shared_ptr<SimObject>*)body->getUserPointer())->reset();
}

Dynamic::~Dynamic()
{
	if (modelInstance)
		delete modelInstance;

	if (body)
	{
		if (body->getUserPointer())
		{
			std::shared_ptr<SimObject>* userDataPtr = (std::shared_ptr<SimObject>*)body->getUserPointer();
			delete userDataPtr;
		}
		world->removeBody(body);
		delete body;
	}
}

std::shared_ptr<Dynamic> dynamicFromBody(const btRigidBody* in)
{
	if (in->getUserIndex() != dynamicBody)
	{
		error("btRigidBody expected to have dynamic type but didn't");
		return nullptr;
	}

	if (!in->getUserPointer())
		return nullptr;

	std::shared_ptr<SimObject>* userDataPointer = (std::shared_ptr<SimObject>*)in->getUserPointer();
	
	if (!(*userDataPointer))
		return nullptr;

	return std::dynamic_pointer_cast<Dynamic>(*userDataPointer);
}
