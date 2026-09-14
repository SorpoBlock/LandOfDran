#include "AddSimObjects.h"

bool AddSimObjectsPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	if (cmdArgs.gameState != InGame)
		return false;

	//Too short to have any objects
	if (packet->dataLength < 4)
		return true;

	switch ((SimObjectType)packet->data[1])
	{
		case StaticTypeId:
		{
			unsigned int numObjects = packet->data[2];
			unsigned int byteIterator = 3;
			for (unsigned int a = 0; a < numObjects; a++)
			{
				netIDType id, type;
				memcpy(&type, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);
				memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);

				glm::vec3 pos;
				glm::quat rot;

				memcpy(&pos, packet->data + byteIterator, sizeof(glm::vec3));
				byteIterator += sizeof(glm::vec3);

				memcpy(&rot, packet->data + byteIterator, sizeof(glm::quat));
				byteIterator += sizeof(glm::quat);

				float friction, restitution;
				memcpy(&friction, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);
				memcpy(&restitution, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);

				unsigned char flags = packet->data[byteIterator];
				byteIterator++;

				std::vector<int> meshIdxs;
				std::vector<glm::vec4> meshColors;

				//Set mesh specific colors for each dynamic
				int numMeshes = packet->data[byteIterator];
				byteIterator++;

				for (int i = 0; i < numMeshes; i++)
				{
					int meshIdx = packet->data[byteIterator];
					byteIterator++;

					glm::vec4 color;
					memcpy(&color.r, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&color.g, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&color.b, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&color.a, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);

					meshIdxs.push_back(meshIdx);
					meshColors.push_back(color);
				}

				bool hasHighlight = packet->data[byteIterator] != 0;
				byteIterator++;

				glm::vec4 highlightColor(0, 0, 0, 0);
				float highlightThickness = 0;
				if (hasHighlight)
				{
					memcpy(&highlightColor.r, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightColor.g, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightColor.b, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightColor.a, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightThickness, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
				}

				std::shared_ptr<DynamicType> foundType = nullptr;
				for (unsigned int i = 0; i < simulation.dynamicTypes.size(); i++)
				{
					if (simulation.dynamicTypes[i]->getID() == type)
					{
						foundType = simulation.dynamicTypes[i];
						break;
					}
				}

				//TODO: Actually return false if we can't find a type with the type ID given
				if (!foundType)
				{
					error("Could not find dynamic type server requested for static object creation.");
					continue;
				}

				/*
					TODO: Server can sometimes send objects twice if they are created right when the client joins
					Once in sendRecentCreations and once in sendAll, this extra check really shouldn't be needed
				*/
				if (simulation.statics->find(id))
					continue;

				simulation.statics->clientSetNextId(id); 
				std::shared_ptr<StaticObject> newStatic = simulation.statics->create(foundType, btVector3(pos.x, pos.y, pos.z), btQuaternion(rot.x, rot.y, rot.z, rot.w));
				newStatic->body->setFriction(friction);
				newStatic->body->setRestitution(restitution);
				 
				if(flags & 1)
					newStatic->setHidden(true);
				else
					newStatic->setHidden(false);
				if (flags & 2)
					newStatic->setColliding(true);
				else
					newStatic->setColliding(false);

				//Set the mesh colors we read earlier
				for (int i = 0; i < meshIdxs.size(); i++)
					newStatic->setMeshColor(meshIdxs[i], meshColors[i]);

				if (hasHighlight)
					newStatic->setHighlight(highlightColor, highlightThickness);

				if (byteIterator >= packet->dataLength)
					break;
			}

			//Point light shadows get redrawn, see LoopClient::renderEverything
			simulation.staticsChanged++;
			break;
		}
		case LightTypeId:
		{
			unsigned int numObjects = packet->data[2];
			unsigned int byteIterator = 3;
			for (unsigned int a = 0; a < numObjects; a++)
			{
				if (byteIterator + sizeof(netIDType) + Light::packetBytes > packet->dataLength)
					break;

				netIDType id;
				memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);

				//Can be sent twice, see the note on statics above
				if (!simulation.lights->find(id))
				{
					simulation.lights->clientSetNextId(id);
					std::shared_ptr<Light> newLight = simulation.lights->create(glm::vec3(0), glm::vec3(1), 0.0f, 0.0f, 0.0f);
					newLight->readFromPacket(packet->data + byteIterator);
				}

				byteIterator += Light::packetBytes;
			}
			break;
		}
		case EmitterTypeId:
		{
			unsigned int numObjects = packet->data[2];
			unsigned int byteIterator = 3;
			for (unsigned int a = 0; a < numObjects; a++)
			{
				if (byteIterator + sizeof(netIDType) + sizeof(uint32_t) + Emitter::packetBytes > packet->dataLength)
					break;

				netIDType id;
				memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);

				uint32_t ageMS;
				memcpy(&ageMS, packet->data + byteIterator, sizeof(uint32_t));
				byteIterator += sizeof(uint32_t);

				//Can be sent twice, see the note on statics above
				if (!simulation.emitters->find(id))
				{
					simulation.emitters->clientSetNextId(id);
					std::shared_ptr<Emitter> newEmitter = simulation.emitters->create((uint16_t)0, glm::vec3(0));
					newEmitter->readFromPacket(packet->data + byteIterator);
					newEmitter->startMS = (int64_t)SDL_GetTicks() - ageMS;
				}

				byteIterator += Emitter::packetBytes;
			}
			break;
		}
		case DynamicTypeId:
		{
			unsigned int numObjects = packet->data[2];
			unsigned int byteIterator = 3;
			for (unsigned int a = 0; a < numObjects; a++)
			{
				netIDType id, type;
				memcpy(&id, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);
				memcpy(&type, packet->data + byteIterator, sizeof(netIDType));
				byteIterator += sizeof(netIDType);
				glm::vec3 pos;
				getPosition(packet->data + byteIterator, pos);
				byteIterator += PositionBytes;
				glm::quat rot;
				getQuaternion(packet->data + byteIterator, rot);
				byteIterator += QuaternionBytes;

				std::shared_ptr<DynamicType> foundType = nullptr;
				for (unsigned int i = 0; i < simulation.dynamicTypes.size(); i++)
				{
					if (simulation.dynamicTypes[i]->getID() == type)
					{
						foundType = simulation.dynamicTypes[i];
						break;
					}
				}

				std::vector<int> meshIdxs;
				std::vector<glm::vec4> meshColors;

				//Set mesh specific colors for each dynamic
				int numMeshes = packet->data[byteIterator];
				byteIterator++;

				for (int i = 0; i < numMeshes; i++)
				{
					int meshIdx = packet->data[byteIterator];
					byteIterator++;

					glm::vec4 color;
					memcpy(&color.r, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&color.g, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&color.b, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&color.a, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);

					meshIdxs.push_back(meshIdx);
					meshColors.push_back(color);
				}

				bool hasHighlight = packet->data[byteIterator] != 0;
				byteIterator++;

				glm::vec4 highlightColor(0, 0, 0, 0);
				float highlightThickness = 0;
				if (hasHighlight)
				{
					memcpy(&highlightColor.r, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightColor.g, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightColor.b, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightColor.a, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
					memcpy(&highlightThickness, packet->data + byteIterator, sizeof(float));
					byteIterator += sizeof(float);
				}

				float buoyancy;
				memcpy(&buoyancy, packet->data + byteIterator, sizeof(float));
				byteIterator += sizeof(float);

				//Faces by mesh index, from Dynamic::setMeshDecal
				std::vector<std::pair<int, int>> meshDecals;
				unsigned int numDecals = packet->data[byteIterator];
				byteIterator++;
				for (unsigned int i = 0; i < numDecals && byteIterator + 2 <= packet->dataLength; i++)
				{
					int meshIdx = packet->data[byteIterator];
					unsigned int nameLength = packet->data[byteIterator + 1];
					byteIterator += 2;
					if (byteIterator + nameLength > packet->dataLength)
						break;

					//A face this game doesn't have is left off
					meshDecals.emplace_back(meshIdx, pd.getFaceDecal(std::string((char*)packet->data + byteIterator, nameLength)));
					byteIterator += nameLength;
				}

				//TODO: Actually return false if we can't find a type with the type ID given
				if (!foundType)
				{
					error("Could not find dynamic type server requested for object creation.");
					continue;
				}

				/*
					TODO: Server can sometimes send objects twice if they are created right when the client joins
					Once in sendRecentCreations and once in sendAll, this extra check really shouldn't be needed
				*/
				if(simulation.dynamics->find(id))
					continue;

				simulation.dynamics->clientSetNextId(id);
				std::shared_ptr<Dynamic> newDynamic = simulation.dynamics->create(foundType, btVector3(pos.x,pos.y,pos.z), btQuaternion(rot.x,rot.y,rot.z,rot.w));

				//Set the mesh colors we read earlier
				for(int i = 0; i < meshIdxs.size(); i++)
					newDynamic->setMeshColor(meshIdxs[i], meshColors[i]);

				if (hasHighlight)
					newDynamic->setHighlight(highlightColor, highlightThickness);

				newDynamic->buoyancy = buoyancy;

				for (const auto& [meshIdx, decalId] : meshDecals)
					newDynamic->setMeshDecal(meshIdx, decalId);

				if (byteIterator >= packet->dataLength)
					break;
			}
			break;
		}
	}

	return true;
}

AddSimObjectsPacket::AddSimObjectsPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

AddSimObjectsPacket::~AddSimObjectsPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
