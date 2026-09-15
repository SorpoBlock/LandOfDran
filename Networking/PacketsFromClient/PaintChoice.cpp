#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"

/*
	See makePaintChoicePacket in ClientPacketCreators.h for the layout

	The color and material the client's paint palette has picked, for Lua's client:getPaintColor and client:getPaintMaterial
*/
void paintChoice(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (packet->dataLength < 6)
		return;

	std::shared_ptr<ClientData> client = pd->getClient(source->me);
	if (!client)
		return;

	memcpy(&client->paintColor[0], packet->data + 1, 4);
	client->paintMaterial = packet->data[5] < BrickMaterialCount ? packet->data[5] : BrickMaterial_None;
}
