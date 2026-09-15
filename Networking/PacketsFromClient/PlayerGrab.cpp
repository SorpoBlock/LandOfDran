#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"

/*
	1 byte		-	packet type

	The client left clicked in game, so their player plays its grab animation for everyone else
	The client already played it on its own player, which ignores the server's updates, see UpdateSimObjectsPacket
*/
void playerGrab(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	//This is only needed because I needed to avoid a circular dependancy by not including SPD in Server.h
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	std::shared_ptr<ClientData> client = pd->getClient(source->me);
	if (!client || client->controllers.empty())
		return;

	std::shared_ptr<Dynamic> player = client->controllers[0].target.lock();
	if (!player)
		return;

	//Lua's addAnimation named it, a model without one just doesn't move
	int grab = player->getType()->getModel()->getAnimationID("grab");
	if (grab != -1)
		player->playOneShot(grab);
}
