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

			unsigned char msSinceLastSend = packet->data[byteIterator];
			byteIterator++;

			unsigned char flags = packet->data[byteIterator];
			byteIterator++;

			bool needPosRot = flags & 1;
			bool needVel = flags & 2;
			bool needAngVel = flags & 4;
			bool needGravity = flags & 8;
			bool needRestitution = flags & 16;
			bool needFriction = flags & 32;
			bool playWalkAnimation = flags & 64;
			bool forcePlayerUpdate = flags & 128;

			glm::vec3 pos;
			glm::quat rot;
			glm::vec3 linVel;
			glm::vec3 angVel;
			glm::vec3 gravity;
			float restitution, friction;
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

			if (needGravity)
			{
				memcpy(&gravity.x, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);
				memcpy(&gravity.y, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);
				memcpy(&gravity.z, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);
			}

			if (needRestitution)
			{
				memcpy(&restitution, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);
			}

			if (needFriction)
			{
				memcpy(&friction, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);
			}

			std::shared_ptr<Dynamic> toUpdate = simulation.dynamics->find(lastId);
			if (toUpdate)
			{
				if (!toUpdate->clientControlled || forcePlayerUpdate)
				{
					if (needPosRot)
					{
						toUpdate->interpolator.addSnapshot(pos, rot, simulation.idealBufferSize, msSinceLastSend);

						btTransform t;
						t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
						t.setOrigin(btVector3(pos.x, pos.y, pos.z));
						toUpdate->body->setWorldTransform(t);
					}

					if (needVel)
						toUpdate->body->setLinearVelocity(btVector3(linVel.x, linVel.y, linVel.z));

					if (needAngVel)
						toUpdate->body->setAngularVelocity(btVector3(angVel.x, angVel.y, angVel.z));

					if (playWalkAnimation)
						toUpdate->play(0, true);
					else
						toUpdate->stop(0);

					if (glm::length(linVel) > 0.1 || glm::length(angVel) > 0.1)
						toUpdate->body->activate();
				}

				if (needGravity)
					toUpdate->body->setGravity(g2b3(gravity));

				if (needRestitution)
					toUpdate->body->setRestitution(restitution);

				if (needFriction)
					toUpdate->body->setFriction(friction);
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
