#include "StaticObject.h"

StaticObject::StaticObject(std::shared_ptr<DynamicType> _type, const btVector3& pos, const btQuaternion& rot)
	: type(_type)
{
	btTransform t;
	t.setIdentity();
	t.setOrigin(pos);
	t.setRotation(rot);

	body = type->createBodyStatic(t); 
	body->forceActivationState(ISLAND_SLEEPING);
	world->addBody(body);

	modelInstance = new ModelInstance(type->getModel().get());
	modelInstance->setModelTransform(glm::translate(b2g3(pos)) * glm::toMat4(glm::quat(rot.w(), rot.x(), rot.y(), rot.z())));
}

void StaticObject::onCreation()
{
	//Pointer to a smart pointer to this
	body->setUserPointer((void*)new std::shared_ptr<SimObject>(getMe()));
}

btVector3 StaticObject::getPosition() const
{
	return body->getWorldTransform().getOrigin();
}

bool StaticObject::requiresNetUpdate() const
{
	return frictionUpdated || restitutionUpdated;
}

unsigned int StaticObject::getUpdatePacketBytes() const
{
	return sizeof(float) * 2; //restitution and friction
}

void StaticObject::addToUpdatePacket(enet_uint8* dest)
{
	float bufF = body->getFriction();
	memcpy(dest, &bufF, sizeof(float));
	bufF = body->getRestitution();
	memcpy(dest + sizeof(float), &bufF, sizeof(float));
	frictionUpdated = false;
	restitutionUpdated = false;
}

void StaticObject::setMeshColor(int meshIdx, const glm::vec4& color)
{
	error("To-do: implement StaticObject::setMeshColor");
	return;
}

ENetPacket* StaticObject::setMeshColor(const std::string& meshName, const glm::vec4& color)
{
	error("To-do: implement StaticObject::setMeshColor");
	return nullptr;
}

/*
	Note, does not include packet header or anything, only the marginal bytes added by* this* object in a bigger packet
	Same goes for all packet related functions here
*/
unsigned int StaticObject::getCreationPacketBytes() const
{
	//Type id, object id, position, rotation, not compressed, friction, restitution
	return sizeof(netIDType) * 2  + sizeof(float) * 9;
}

void StaticObject::addToCreationPacket(enet_uint8* dest) const
{
	netIDType buf = type->getID();
	memcpy(dest, &buf, sizeof(netIDType));
	buf = getID();
	memcpy(dest + sizeof(netIDType), &buf, sizeof(netIDType));
	glm::vec3 pos = b2g3(body->getWorldTransform().getOrigin());
	memcpy(dest + sizeof(netIDType) * 2, &pos, sizeof(glm::vec3));
	btQuaternion bQuat = body->getWorldTransform().getRotation();
	glm::quat gQuat = glm::quat(bQuat.getW(), bQuat.getX(), bQuat.getY(), bQuat.getZ());
	memcpy(dest + sizeof(netIDType) * 2 + sizeof(glm::vec3), &gQuat, sizeof(glm::quat));
	float bufF = body->getFriction();
	memcpy(dest + sizeof(netIDType) * 2 + sizeof(glm::vec3) + sizeof(glm::quat), &bufF, sizeof(float));
	bufF = body->getRestitution();
	memcpy(dest + sizeof(netIDType) * 2 + sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(float), &bufF, sizeof(float));
}

void StaticObject::requestDestruction()
{
	((std::shared_ptr<SimObject>*)body->getUserPointer())->reset();
}

StaticObject::~StaticObject()
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

std::shared_ptr<StaticObject> staticFromBody(const btRigidBody* in)
{
	if (in->getUserIndex() != staticBody)
	{
		error("btRigidBody expected to have static type but didn't");
		return nullptr;
	}

	if (!in->getUserPointer())
		return nullptr;

	std::shared_ptr<SimObject>* userDataPointer = (std::shared_ptr<SimObject>*)in->getUserPointer();

	if (!(*userDataPointer))
		return nullptr;

	return std::dynamic_pointer_cast<StaticObject>(*userDataPointer);
}
