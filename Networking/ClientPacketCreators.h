#pragma once

#include "../LandOfDran.h"
#include "../Bricks/BrickHolder.h"
#include "../GameLoop/PlayerAppearance.h"

/*
	Global inline functions that help create miscellaneous packet types from passed parameters
	These are for packets sent to the server from the client
*/

inline ENetPacket* makeMouseClickPacket(glm::vec3 pos, glm::vec3 dir, unsigned char mask)
{
	ENetPacket* ret = enet_packet_create(NULL, 2 + 6 * sizeof(float), getFlagsFromChannel(OtherReliable));

	ret->data[0] = (unsigned char)ClickDetails;
	memcpy(ret->data + 1 + sizeof(float) * 0, &pos.x, sizeof(float));
	memcpy(ret->data + 1 + sizeof(float) * 1, &pos.y, sizeof(float));
	memcpy(ret->data + 1 + sizeof(float) * 2, &pos.z, sizeof(float));
	memcpy(ret->data + 1 + sizeof(float) * 3, &dir.x, sizeof(float));
	memcpy(ret->data + 1 + sizeof(float) * 4, &dir.y, sizeof(float));
	memcpy(ret->data + 1 + sizeof(float) * 5, &dir.z, sizeof(float));
	ret->data[1 + sizeof(float) * 6] = mask;
	
	return ret;
}

/*
	1 byte		-	packet type
	1 byte		-	client game version
	1 byte		-	name length, max 255
	1-255 bytes	-	name
*/
inline ENetPacket* makeConnectionRequest(std::string name)
{
	ENetPacket* ret = enet_packet_create(NULL, name.length() + 3, getFlagsFromChannel(JoinNegotiation));

	ret->data[0] = (unsigned char)ConnectionRequest;
	ret->data[1] = (unsigned char)GAME_VERSION;
	ret->data[2] = (unsigned char)name.length();
	memcpy(ret->data + 3, name.c_str(), name.length());

	return ret;
}

/*
	1 byte		-		packet type
	4 bytes		-		controlled dynamic id
	1 byte		-		movement control flags
	4 bytes		-		camera x direction
	4 bytes		-		camera y direction
	4 bytes		-		camera z direction
	4 bytes		-		camera x position
	4 bytes		-		camera y position
	4 bytes		-		camera z position
*/
inline ENetPacket* makeMovementInputs(netIDType controlledDynamicID, bool jump, bool jumpHeld, bool forward,bool backward,bool left,bool right, bool jet, glm::vec3 cameraDirection, glm::vec3 cameraPosition)
{
	//Unreliable and resent every interval regardless of whether the state changed (see PlayerController::makeMovementInputsPacket) -
	//losing any single one just means the server acts on a stale input state for one more interval before the next resend corrects it,
	//rather than risking head-of-line blocking other client->server reliable traffic under sustained loss
	ENetPacket* ret = enet_packet_create(NULL, 30, getFlagsFromChannel(Unreliable));

	unsigned char movementFlags = 0;
	movementFlags |= (jump ? MovementFlag_Jump : 0);
	movementFlags |= (forward ? MovementFlag_Forward : 0);
	movementFlags |= (backward ? MovementFlag_Backward : 0);
	movementFlags |= (left ? MovementFlag_Left : 0);
	movementFlags |= (right ? MovementFlag_Right : 0);
	movementFlags |= (jumpHeld ? MovementFlag_JumpHeld : 0);
	movementFlags |= (jet ? MovementFlag_Jet : 0);

	ret->data[0] = (unsigned char)MovementInputs;
	memcpy(ret->data + 1, &controlledDynamicID, sizeof(netIDType));
	ret->data[1 + sizeof(netIDType)] = movementFlags;
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 0, &cameraDirection.x, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 1, &cameraDirection.y, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 2, &cameraDirection.z, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 3, &cameraPosition.x, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 4, &cameraPosition.y, sizeof(float));
	memcpy(ret->data + 1 + sizeof(netIDType) + 1 + sizeof(float) * 5, &cameraPosition.z, sizeof(float));

	return ret;
}

//1 byte packet type, then a brick record (see BrickHolder::writeRecord) whose id the server ignores
inline ENetPacket* makePlantBrickPacket(const Brick& brick)
{
	ENetPacket* ret = enet_packet_create(NULL, 1 + BrickHolder::recordBytes, getFlagsFromChannel(OtherReliable));
	ret->data[0] = (unsigned char)PlantBrickRequest;
	BrickHolder::writeRecord(&brick, ret->data + 1);
	return ret;
}

inline ENetPacket* makeUndoBrickPacket()
{
	char data = UndoBrickRequest;
	return enet_packet_create(&data, 1, getFlagsFromChannel(OtherReliable));
}

/*
	1 byte   - packet type
	1 byte   - flags, see VoiceFlag_End
	2 bytes  - sequence number, one more for each 20 ms frame
	The rest - one Opus frame, up to maxVoiceFrameBytes, nothing with VoiceFlag_End
*/
inline ENetPacket* makeVoiceFramePacket(unsigned char flags, uint16_t sequence, const unsigned char* frame, unsigned int length)
{
	length = std::min(length, maxVoiceFrameBytes);

	ENetPacket* ret = enet_packet_create(NULL, 2 + sizeof(uint16_t) + length, getFlagsFromChannel(VoiceData));
	ret->data[0] = (unsigned char)VoiceFrame;
	ret->data[1] = flags;
	memcpy(ret->data + 2, &sequence, sizeof(uint16_t));
	if (length > 0)
		memcpy(ret->data + 2 + sizeof(uint16_t), frame, length);

	return ret;
}

//One byte lets the server know we finished phase one loading
inline ENetPacket* makeLoadingFinished()
{
	char theSmallestPacketEver[1];
	theSmallestPacketEver[0] = LoadingFinished;
	ENetPacket* ret = enet_packet_create(theSmallestPacketEver, 1, getFlagsFromChannel(JoinNegotiation));
	return ret;
}

//Send a chat message to the server
inline ENetPacket* makeChatMessage(const std::string &message)
{
	ENetPacket* ret = enet_packet_create(NULL, message.length() + 2, getFlagsFromChannel(OtherReliable));

	ret->data[0] = (unsigned char)ChatMessage;
	ret->data[1] = (unsigned char)message.length();
	memcpy(ret->data + 2, message.c_str(), message.length());

	return ret;
}

//Try to log into the server's eval console
inline ENetPacket* attemptEvalLogin(const std::string &password)
{
	ENetPacket* ret = enet_packet_create(NULL, password.length() + 2, getFlagsFromChannel(OtherReliable));
	ret->data[0] = (unsigned char)EvalLogin;
	ret->data[1] = (unsigned char)password.length();
	memcpy(ret->data + 2, password.c_str(), password.length());

	return ret;
}

inline ENetPacket *evalCommand(const std::string& password, const std::string &command)
{
	ENetPacket* ret = enet_packet_create(NULL, command.length() + password.length() + 4, getFlagsFromChannel(OtherReliable));
	ret->data[0] = (unsigned char)EvalCommand;

	unsigned short commandLength = command.length();
	memcpy(ret->data + 1, &commandLength, sizeof(unsigned short));

	memcpy(ret->data + 3, command.c_str(), command.length());
	ret->data[command.length() + 3] = (unsigned char)password.length();
	memcpy(ret->data + command.length() + 4, password.c_str(), password.length());
	//TODO: Hash password

	return ret;
}

/*
	1 byte		-	packet type
	1 byte		-	1 to turn the flashlight on, 0 for off
	12 bytes	-	red, green, blue floats, 0-1
*/
inline ENetPacket* makeFlashlightPacket(bool on, glm::vec3 color)
{
	ENetPacket* ret = enet_packet_create(NULL, 2 + sizeof(float) * 3, getFlagsFromChannel(OtherReliable));

	ret->data[0] = (unsigned char)FlashlightRequest;
	ret->data[1] = on ? 1 : 0;
	memcpy(ret->data + 2, &color[0], sizeof(float) * 3);

	return ret;
}

/*
	1 byte		-	packet type
*/
inline ENetPacket* makePlayerGrabPacket()
{
	ENetPacket* ret = enet_packet_create(NULL, 1, getFlagsFromChannel(Unreliable));
	ret->data[0] = (unsigned char)PlayerGrab;
	return ret;
}

/*
	1 byte		-	packet type
	12 bytes	-	camera position
	12 bytes	-	camera direction
*/
inline ENetPacket* makeWrenchRequestPacket(glm::vec3 position, glm::vec3 direction)
{
	ENetPacket* ret = enet_packet_create(NULL, 1 + sizeof(float) * 6, getFlagsFromChannel(OtherReliable));

	ret->data[0] = (unsigned char)WrenchRequest;
	memcpy(ret->data + 1, &position[0], sizeof(float) * 3);
	memcpy(ret->data + 1 + sizeof(float) * 3, &direction[0], sizeof(float) * 3);

	return ret;
}

/*
	1 byte		-	packet type
	4 bytes		-	brick net ID, from the WrenchDialog packet
	1 byte		-	1 if it collides
	1 byte		-	name length
	0-255 bytes	-	name
	The rest	-	BrickAttachments::write
*/
inline ENetPacket* makeWrenchSubmitPacket(netIDType brickID, bool collides, const std::string& name, const BrickAttachments& attachments)
{
	std::string shortName = name.substr(0, 255);

	std::vector<unsigned char> bytes;
	bytes.push_back((unsigned char)WrenchSubmit);
	bytes.resize(1 + sizeof(netIDType));
	memcpy(bytes.data() + 1, &brickID, sizeof(netIDType));
	bytes.push_back(collides ? 1 : 0);
	bytes.push_back((unsigned char)shortName.length());
	bytes.insert(bytes.end(), shortName.begin(), shortName.end());
	attachments.write(bytes);

	return enet_packet_create(bytes.data(), bytes.size(), getFlagsFromChannel(OtherReliable));
}

/*
	1 byte		-	packet type
	1 byte		-	face name length, 0 for no face
	0-64 bytes	-	face name, a file in Assets/faces
	1 byte		-	how many painted parts follow
	Per part:
	1 byte		-	mesh name length
	1-64 bytes	-	mesh name
	3 bytes		-	red, green, blue, 0-255
*/
inline ENetPacket* makeAppearanceChoicePacket(const PlayerAppearance& appearance)
{
	std::string face = appearance.face.substr(0, PlayerAppearance::maxNameLength);

	std::vector<std::pair<std::string, glm::vec3>> colors;
	for (const auto& [meshName, color] : appearance.colors)
	{
		if (colors.size() < PlayerAppearance::maxColors && !meshName.empty())
			colors.emplace_back(meshName.substr(0, PlayerAppearance::maxNameLength), color);
	}

	size_t length = 3 + face.length();
	for (const auto& [meshName, color] : colors)
		length += 4 + meshName.length();

	ENetPacket* ret = enet_packet_create(NULL, length, getFlagsFromChannel(JoinNegotiation));

	ret->data[0] = (unsigned char)AppearanceChoice;
	ret->data[1] = (unsigned char)face.length();
	memcpy(ret->data + 2, face.data(), face.length());

	size_t byteIterator = 2 + face.length();
	ret->data[byteIterator++] = (unsigned char)colors.size();
	for (const auto& [meshName, color] : colors)
	{
		ret->data[byteIterator++] = (unsigned char)meshName.length();
		memcpy(ret->data + byteIterator, meshName.data(), meshName.length());
		byteIterator += meshName.length();

		glm::vec3 bytes = glm::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f;
		for (int channel = 0; channel < 3; channel++)
			ret->data[byteIterator++] = (unsigned char)bytes[channel];
	}

	return ret;
}
