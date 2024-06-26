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
	return PositionBytes + QuaternionBytes + sizeof(netIDType);
}

void Dynamic::setPosition(const btVector3& pos)
{
	btTransform t = body->getWorldTransform();
	t.setOrigin(pos);
	body->setWorldTransform(t);
}

void Dynamic::setVelocity(const btVector3& vel)
{
	body->setLinearVelocity(vel);
}

btVector3 Dynamic::getVelocity()
{
	return body->getLinearVelocity();
}

void Dynamic::addTransform(glm::vec3 pos, glm::quat rot)
{
	//TODO: Snapshot interpolator
	modelInstance->setModelTransform(glm::translate(pos) * glm::toMat4(rot));
}

bool Dynamic::requiresNetUpdate() const
{
	//For dynamics requiresUpdate means a change to something like a decal, or a node color
	if (requiresUpdate)
		return true;

	const btTransform& t = body->getWorldTransform();

	//If the object has moved more than 0.14 studs
	if (t.getOrigin().distance2(lastSentTransform.getOrigin()) > 0.02)
		return true;

	//More than like 8 degrees difference in rotation?
	if (t.getRotation().dot(lastSentTransform.getRotation()) < 0.99)
		return true;

	//Even if the object isn't moving much at all we should still send out an update every once in a while
	if (SDL_GetTicks() - lastSentTime > 500)
		return true;

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

	memcpy(dest, &netID, sizeof(netIDType));
	addPosition(dest + sizeof(netIDType), pos);
	addQuaternion(dest + PositionBytes + sizeof(netIDType), quat);
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
