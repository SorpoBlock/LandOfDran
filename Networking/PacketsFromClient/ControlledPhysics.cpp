#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../GameLoop/ClientData.h"

/*
	Do not attempt to assign a handle to JoinedClient to other objects directly
	Grab a smart pointer from the server for this
	server->getClientByNetId(source->getNetId());
	Also do not attempt to delete the JoinedClient, just call kick
*/
void applyPhysicsAdjustment(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if(packet->dataLength < (1 + sizeof(netIDType) + PositionBytes + VelocityBytes + AngularVelocityBytes + QuaternionBytes))
	{
		std::cout << "Packet too small to be a physics adjustment packet" << std::endl;
		return;
	}

	int byteIterator = 1;
	netIDType id;
	memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
	byteIterator += sizeof(netIDType);
	glm::vec3 pos;
	getPosition(packet->data + byteIterator, pos);
	byteIterator += PositionBytes;
	glm::quat rot;
	getQuaternion(packet->data + byteIterator, rot);
	byteIterator += QuaternionBytes;
	glm::vec3 linVel;
	getVelocity(packet->data + byteIterator, linVel);
	byteIterator += VelocityBytes;
	glm::vec3 angVel;
	getAngularVelocity(packet->data + byteIterator, angVel);
	byteIterator += AngularVelocityBytes;

	std::shared_ptr<Dynamic> toUpdate = pd->dynamics->find(id);

	if (!toUpdate)
		return;

	btTransform t;
	t.setOrigin(btVector3(pos.x, pos.y, pos.z));
	t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
	toUpdate->body->setWorldTransform(t);

	toUpdate->body->setLinearVelocity(btVector3(linVel.x, linVel.y, linVel.z));
	toUpdate->body->setAngularVelocity(btVector3(angVel.x, angVel.y, angVel.z));
}