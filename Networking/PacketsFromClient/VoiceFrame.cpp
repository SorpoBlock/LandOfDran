#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../LuaFunctions/ClientLua.h"

//The dynamic a client's voice comes from: whatever their movement keys control, otherwise the first dynamic they simulate
static std::shared_ptr<Dynamic> playerOf(const ClientData& client)
{
	if (!client.controllers.empty())
	{
		if (std::shared_ptr<Dynamic> target = client.controllers[0].target.lock())
			return target;
	}

	if (!client.controlledObjects.empty())
		return client.controlledObjects[0];

	return nullptr;
}

//Where a client hears voices from: their camera, otherwise their player. False if there's no telling
static bool listenerPosition(const ClientData& client, glm::vec3& position)
{
	if (!client.controllers.empty())
	{
		position = client.controllers[0].lastCameraPosition;
		return true;
	}

	if (std::shared_ptr<Dynamic> player = playerOf(client))
	{
		position = b2g3(player->getPosition());
		return true;
	}

	return false;
}

/*
	Do not attempt to assign a handle to JoinedClient to other objects directly
	Grab a smart pointer from the server for this
	server->getClientByNetId(source->getNetId());
	Also do not attempt to delete the JoinedClient, just call kick
*/
void voiceFrame(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	/*
		1 byte   - packet type
		1 byte   - flags, see VoiceFlag_End
		2 bytes  - sequence number, one more for each 20 ms frame
		The rest - one Opus frame, nothing with VoiceFlag_End
	*/
	const unsigned int headerBytes = 2 + sizeof(uint16_t);
	if (packet->dataLength < headerBytes || packet->dataLength > headerBytes + maxVoiceFrameBytes)
		return;

	std::shared_ptr<ClientData> talker = pd->getClient(source->me);
	if (!talker || talker->voiceMuted)
		return;

	unsigned char flags = packet->data[1];
	bool ending = flags & VoiceFlag_End;

	talker->lastVoiceMS = SDL_GetTicks();

	//Starting after being quiet, or saying they're done. LoopServer::endQuietTalkers catches the ones that go quiet without saying so
	if (talker->talking == ending)
	{
		talker->talking = !ending;

		pushClientLua(pd->luaState, source->me);
		pd->eventManager->callEvent(pd->luaState, ending ? "ClientStopTalking" : "ClientStartTalking", 1);
		lua_settop(pd->luaState, 0);

		if (talker->voiceMuted)
			return;
	}

	if (pd->voiceRange <= 0)
		return;

	SoundLocationKind kind;
	glm::vec3 origin;
	std::shared_ptr<Dynamic> player = playerOf(*talker);
	if (player)
	{
		kind = SoundLocationDynamic;
		origin = b2g3(player->getPosition());
	}
	else if (!talker->controllers.empty())
	{
		kind = SoundLocationFixed;
		origin = talker->controllers[0].lastCameraPosition;
	}
	else
		return; //Nowhere to hear them from

	/*
		1 byte   - packet type
		4 bytes  - net ID of the client talking
		1 byte   - flags
		2 bytes  - sequence number
		Where the talker is, see readSoundLocation in OneShotSound.cpp
		The rest - the Opus frame
	*/
	netIDType talkerID = source->getNetId();
	unsigned int locationBytes = 1 + (kind == SoundLocationFixed ? sizeof(float) * 3 : sizeof(netIDType));
	unsigned int frameBytes = (unsigned int)packet->dataLength - headerBytes;
	unsigned int relayedBytes = 1 + sizeof(netIDType) + 1 + sizeof(uint16_t) + locationBytes + frameBytes;

	//One packet shared by every listener, ENet counts how many peers it was sent to
	ENetPacket* relayed = enet_packet_create(NULL, relayedBytes, getFlagsFromChannel(VoiceData));
	unsigned int byteIterator = 0;
	relayed->data[byteIterator] = (unsigned char)VoiceFrameFromServer;
	byteIterator += 1;
	memcpy(relayed->data + byteIterator, &talkerID, sizeof(netIDType));
	byteIterator += sizeof(netIDType);
	memcpy(relayed->data + byteIterator, packet->data + 1, 1 + sizeof(uint16_t));
	byteIterator += 1 + sizeof(uint16_t);
	relayed->data[byteIterator] = kind;
	byteIterator += 1;
	if (kind == SoundLocationFixed)
	{
		memcpy(relayed->data + byteIterator, &origin.x, sizeof(float));
		memcpy(relayed->data + byteIterator + sizeof(float), &origin.y, sizeof(float));
		memcpy(relayed->data + byteIterator + sizeof(float) * 2, &origin.z, sizeof(float));
		byteIterator += sizeof(float) * 3;
	}
	else
	{
		netIDType playerID = player->getID();
		memcpy(relayed->data + byteIterator, &playerID, sizeof(netIDType));
		byteIterator += sizeof(netIDType);
	}
	memcpy(relayed->data + byteIterator, packet->data + headerBytes, frameBytes);

	for (const std::shared_ptr<ClientData>& listener : pd->clients)
	{
		if (listener == talker || !listener->client)
			continue;

		glm::vec3 heardFrom;
		if (!listenerPosition(*listener, heardFrom) || glm::distance(heardFrom, origin) > pd->voiceRange)
			continue;

		//So they can show who's talking
		if (listener->knownTalkers.insert(talkerID).second)
		{
			unsigned int nameLength = (unsigned int)std::min(source->name.length(), (size_t)255);
			ENetPacket* name = enet_packet_create(NULL, 2 + sizeof(netIDType) + 1 + nameLength, getFlagsFromChannel(OtherReliable));
			name->data[0] = (unsigned char)VoiceStatus;
			name->data[1] = (unsigned char)VoiceStatusTalkerName;
			memcpy(name->data + 2, &talkerID, sizeof(netIDType));
			name->data[2 + sizeof(netIDType)] = (unsigned char)nameLength;
			memcpy(name->data + 3 + sizeof(netIDType), source->name.c_str(), nameLength);
			listener->client->send(name, OtherReliable);
		}

		listener->client->send(relayed, VoiceData);
	}

	if (relayed->referenceCount == 0)
		enet_packet_destroy(relayed);
}
