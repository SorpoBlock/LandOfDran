#include "DynamicType.h"

/*
	1 byte			- packet type
	1 byte			- SimObject type
	4 bytes			- object id
	1 byte			- relative file path to model length
	1-255 bytes		- relative file path to model
*/
void DynamicType::loadFromPacket(ENetPacket const* const packet)
{
	//packet type and simobject type were already processed to get to this point

	//Packet isn't even big enough to have a 1 character file path
	if (packet->dataLength < (sizeof(netIDType) + 4))
		return;

	//Get id of object
	memcpy(&id, packet->data + 2, sizeof(netIDType));

	unsigned int filePathLen = packet->data[2 + sizeof(netIDType)];

	//Packet not long enough for file path string
	if (packet->dataLength < 3 + sizeof(netIDType) + filePathLen)
		return;

	std::string filePath = std::string((char*)packet->data + 3 + sizeof(netIDType), filePathLen);

	model = std::make_shared<Model>(filePath);
}

//Create the packet on the server to be passed to loadFromPacket on the client
ENetPacket* DynamicType::createTypePacket() const
{
	ENetPacket* ret = enet_packet_create(NULL, model->loadedPath.length() + sizeof(netIDType) + 3, getFlagsFromChannel(OtherReliable));

	ret->data[0] = AddSimObjectType;
	ret->data[1] = DynamicTypeId;
	memcpy(ret->data + 2, &id, sizeof(netIDType));
	ret->data[2 + sizeof(netIDType)] = model->loadedPath.length();
	memcpy(ret->data + 3 + sizeof(netIDType), model->loadedPath.c_str(), model->loadedPath.length());

	return ret; 
}

//Doesn't allocate anything, wait for loadFromPacket or server-side construction from lua
DynamicType::DynamicType() : NetType(DynamicTypeId)
{

}

//Deallocates model if it exists
DynamicType::~DynamicType()
{
	if (model)
	{
		//Not needed this is a destructor lol
		model.reset();
		model = nullptr;
	}
}

