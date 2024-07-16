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

	if(packet->dataLength < (1 + sizeof(netIDType)))
	{
		std::cout << "Packet too small to be a physics adjustment packet" << std::endl;
		return;
	}

	int byteIterator = 1;
	netIDType id;
	memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
	byteIterator += sizeof(netIDType);

	unsigned char flags = packet->data[byteIterator];
	byteIterator++;

	bool needPosRot = flags & 1;
	bool needVel = flags & 2;
	bool needAngVel = flags & 4;

	unsigned neededSize = 1 + sizeof(netIDType);
	neededSize += needPosRot ? PositionBytes + QuaternionBytes : 0;
	neededSize += needVel ? VelocityBytes : 0;
	neededSize += needAngVel ? AngularVelocityBytes : 0;

	if(packet->dataLength < neededSize)
	{
		std::cout << "Packet too small to be a physics adjustment packet" << std::endl;
		return;
	}

	glm::vec3 pos;
	glm::quat rot;
	glm::vec3 linVel;
	glm::vec3 angVel;

	if (needPosRot)
	{
		getPosition(packet->data + byteIterator, pos);
		byteIterator += PositionBytes;
		getQuaternion(packet->data + byteIterator, rot);
		byteIterator += QuaternionBytes;
	}

	if (needVel)
	{
		getVelocity(packet->data + byteIterator, linVel);
		byteIterator += VelocityBytes;
	}

	if (needAngVel)
	{
		getAngularVelocity(packet->data + byteIterator, angVel);
		byteIterator += AngularVelocityBytes;
	}

	std::shared_ptr<Dynamic> toUpdate = pd->dynamics->find(id);

	if (!toUpdate)
		return;

	if (needPosRot)
	{
		btTransform t;
		t.setOrigin(btVector3(pos.x, pos.y, pos.z));
		t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
		toUpdate->body->setWorldTransform(t);
	}

	if(needVel)
		toUpdate->body->setLinearVelocity(btVector3(linVel.x, linVel.y, linVel.z));
	if(needAngVel)
		toUpdate->body->setAngularVelocity(btVector3(angVel.x, angVel.y, angVel.z));
}