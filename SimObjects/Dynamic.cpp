#include "Dynamic.h"

Dynamic::Dynamic(std::shared_ptr<DynamicType> _type, const btVector3& initialPos, const btQuaternion &initialRot) : type(_type)
{
	body = type->createBody();
	world->addBody(body);

	btTransform t;
	t.setIdentity();
	t.setOrigin(initialPos);
	t.setRotation(initialRot);
	body->setWorldTransform(t);

	modelInstance = new ModelInstance(type->getModel().get());

	if (!type->getModel()->isServerSide())
	{
		//TODO: Pass initial rotation as well
		interpolator.addSnapshot(b2g3(initialPos), glm::quat(initialRot.w(),initialRot.x(),initialRot.y(),initialRot.z()), 4);
		modelInstance->setModelTransform(glm::translate(glm::vec3(initialPos.x(), initialPos.y(), initialPos.z())));
	}
}

void Dynamic::onCreation()
{
	//Pointer to a smart pointer to this
	body->setUserPointer((void*)new std::shared_ptr<SimObject>(getMe()));
}

void Dynamic::updateSnapshot(bool forceUsePhysicsTransform)
{
	if (clientControlled || forceUsePhysicsTransform)
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

	if (body->getWorldTransform().getRotation().angleShortestPath(lastSentTransform.getRotation()) > 0.01)
		return true;

	if (lastSentVel.distance2(body->getLinearVelocity()) > 0.37)
		return true;

	//More than like 6 degrees difference in rotation?
	if (lastSentAngVel.distance2(body->getAngularVelocity()) > 0.37)
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

unsigned int Dynamic::getUpdatePacketBytes() const
{
	/*
		4 bytes - net ID
		1 byte what needs updating
		position
		rotation
		velocity
		angular velocity
	*/

	unsigned int ret = 1;

	const btTransform& t = body->getWorldTransform();

	bool needPosRot = false;

	//If the object has moved more than 0.14 studs
	if (t.getOrigin().distance2(lastSentTransform.getOrigin()) > 0.005)
		needPosRot = true;

	if (body->getWorldTransform().getRotation().angleShortestPath(lastSentTransform.getRotation()) > 0.01)
		needPosRot = true;

	if (needPosRot)
	{
		ret += PositionBytes;
		ret += QuaternionBytes;
	}

	if (lastSentVel.distance2(body->getLinearVelocity()) > 0.37)
		ret += VelocityBytes;

	//More than like 6 degrees difference in rotation?
	if (lastSentAngVel.distance2(body->getAngularVelocity()) > 0.37)
		ret += AngularVelocityBytes;

	return ret;
}

void Dynamic::setMeshColor(int meshIdx, const glm::vec4& color)
{
	modelInstance->setColor(meshIdx, color);
}

ENetPacket* Dynamic::setMeshColor(const std::string &meshName,const glm::vec4& color)
{
	int meshIdx = getType()->getModel()->getMeshIdx(meshName);
	if(meshIdx == -1)
	{
		error("Mesh " + meshName + " not found in model");
		return nullptr;
	} 

	modelInstance->setColor(meshIdx, color);

	ENetPacket *ret = enet_packet_create(NULL, sizeof(netIDType) + 2 + sizeof(glm::vec4), getFlagsFromChannel(OtherReliable));

	ret->data[0] = (unsigned char)MeshAppearance;
	memcpy(ret->data + 1, &netID, sizeof(netIDType));
	ret->data[sizeof(netIDType) + 1] = meshIdx;
	memcpy(ret->data + sizeof(netIDType) + 2 + sizeof(float) * 0, &color.r, sizeof(float));
	memcpy(ret->data + sizeof(netIDType) + 2 + sizeof(float) * 1, &color.g, sizeof(float));
	memcpy(ret->data + sizeof(netIDType) + 2 + sizeof(float) * 2, &color.b, sizeof(float));	
	memcpy(ret->data + sizeof(netIDType) + 2 + sizeof(float) * 3, &color.a, sizeof(float));

	return ret;
}

void Dynamic::addToUpdatePacket(enet_uint8 * dest)
{
	requiresUpdate = false;

	const btTransform& t = body->getWorldTransform();

	bool needPosRot = false;
	if (t.getOrigin().distance2(lastSentTransform.getOrigin()) > 0.005)
		needPosRot = true;
	if (body->getWorldTransform().getRotation().angleShortestPath(lastSentTransform.getRotation()) > 0.01)
		needPosRot = true;

	bool needVel = false;
	if (lastSentVel.distance2(body->getLinearVelocity()) > 0.37)
		needVel = true;

	bool needAngVel = false;
	if (lastSentAngVel.distance2(body->getAngularVelocity()) > 0.37)
		needAngVel = true;

	lastSentTime = SDL_GetTicks();

	//Flags saying what was updated
	unsigned char flags = 0;
	flags += needPosRot ? 1 : 0;
	flags += needVel    ? 2 : 0;
	flags += needAngVel ? 4 : 0;
	dest[0] = flags;

	int byteIterator = 1;

	if (needPosRot)
	{
		lastSentTransform = body->getWorldTransform();
		const glm::vec3& pos = glm::vec3(lastSentTransform.getOrigin().x(), lastSentTransform.getOrigin().y(), lastSentTransform.getOrigin().z());
		const glm::quat& quat = glm::quat(lastSentTransform.getRotation().w(), lastSentTransform.getRotation().x(), lastSentTransform.getRotation().y(), lastSentTransform.getRotation().z());
		addPosition(dest + byteIterator, pos);
		byteIterator += PositionBytes;
		addQuaternion(dest + byteIterator, quat);
		byteIterator += QuaternionBytes;
	}
	if (needVel)
	{
		lastSentVel = body->getLinearVelocity();
		const glm::vec3& linVel = glm::vec3(body->getLinearVelocity().x(), body->getLinearVelocity().y(), body->getLinearVelocity().z());
		addVelocity(dest + byteIterator, linVel);
		byteIterator += VelocityBytes;
	}
	if (needAngVel)
	{
		lastSentAngVel = body->getAngularVelocity();
		const glm::vec3& angVel = glm::vec3(body->getAngularVelocity().x(), body->getAngularVelocity().y(), body->getAngularVelocity().z());
		addAngularVelocity(dest + byteIterator, angVel);
		byteIterator += AngularVelocityBytes;
	}
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

	int meshColorsSize = 0;
	glm::vec4 color;
	for (int a = 0; a < modelInstance->getNumMeshes(); a++)
		if (modelInstance->getMeshColor(a, color))
			meshColorsSize++;

	meshColorsSize *= sizeof(glm::vec4) + 1; //1 byte for mesh index
	meshColorsSize++; //1 extra byte for how many mesh colors we are sending

	return meshColorsSize + PositionBytes + QuaternionBytes + sizeof(netIDType) * 2;
}

void Dynamic::addToCreationPacket(enet_uint8* dest) const
{
	memcpy(dest, &netID, sizeof(netIDType));

	const netIDType typeID = type->getID();
	memcpy(dest + sizeof(netIDType), &typeID, sizeof(netIDType));

	const btTransform& t = body->getWorldTransform();
	const glm::vec3& pos = glm::vec3(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z());
	const glm::quat& quat = glm::quat(t.getRotation().w(), t.getRotation().x(), t.getRotation().y(), t.getRotation().z());

	addPosition(dest + sizeof(netIDType) * 2, pos);
	addQuaternion(dest + sizeof(netIDType) * 2 + PositionBytes, quat);

	int meshColorsStart = sizeof(netIDType) * 2 + PositionBytes + QuaternionBytes;

	//Skip the byte with the mesh colors count for now until we actually know how many meshes need colors sent
	int byteIterator = meshColorsStart+1; 

	int meshColors = 0;
	glm::vec4 color;
	for (int a = 0; a < modelInstance->getNumMeshes(); a++)
	{
		if (modelInstance->getMeshColor(a, color))
		{
			dest[byteIterator] = a;
			byteIterator++;

			memcpy(dest + byteIterator, &color.r, sizeof(float));
			byteIterator += sizeof(float);
			memcpy(dest + byteIterator, &color.g, sizeof(float));
			byteIterator += sizeof(float);
			memcpy(dest + byteIterator, &color.b, sizeof(float));
			byteIterator += sizeof(float);
			memcpy(dest + byteIterator, &color.a, sizeof(float));
			byteIterator += sizeof(float);

			meshColors++;
		}
	}

	//Now we know how many meshes need updating
	dest[meshColorsStart] = meshColors;
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
