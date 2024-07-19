#include "UpdateSimObjects.h"

bool UpdateSimObjectsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	//TODO: Have server not send updates until we confirm we've loaded in
	if (cmdArgs.gameState != InGame)
		return true;

	//Too short to have any objects
	if (packet->dataLength < 4)
		return true;

	switch ((SimObjectType)packet->data[1])
	{
	case DynamicTypeId:
	{
		unsigned int numObjects = packet->data[2];
		unsigned int byteIterator = 3;
		netIDType lastId = NO_ID;
		for (unsigned int a = 0; a < numObjects; a++)
		{
			lastId = simulation.dynamics->getIdFromDelta(packet->data + byteIterator, lastId, byteIterator);

			unsigned char flags = packet->data[byteIterator];
			byteIterator++;

			bool needPosRot = flags & 1;
			bool needVel = flags & 2;
			bool needAngVel = flags & 4;

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

			std::shared_ptr<Dynamic> toUpdate = simulation.dynamics->find(lastId);
			if (toUpdate && !toUpdate->clientControlled)
			{
				if (needPosRot)
				{
					toUpdate->interpolator.addSnapshot(pos, rot, simulation.idealBufferSize);

					btTransform t;
					t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
					t.setOrigin(btVector3(pos.x, pos.y, pos.z));
					toUpdate->body->setWorldTransform(t);
				}

				if(needVel)
					toUpdate->body->setLinearVelocity(btVector3(linVel.x, linVel.y, linVel.z));

				if(needAngVel)
					toUpdate->body->setAngularVelocity(btVector3(angVel.x, angVel.y, angVel.z));

				if (glm::length(linVel) > 0.1 || glm::length(angVel) > 0.1)
					toUpdate->body->activate();
			}

			if (byteIterator >= packet->dataLength)
				break;
		}
		break;
	}
	}

	return true;
}

UpdateSimObjectsPacket::UpdateSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

UpdateSimObjectsPacket::~UpdateSimObjectsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
