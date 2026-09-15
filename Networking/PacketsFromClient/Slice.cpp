#include "../Server.h"
#include "../../GameLoop/ServerProgramData.h"
#include "../../LuaFunctions/VehicleLua.h"
#include "../../Bricks/SelectionBox.h"

//Milliseconds a client waits between slices, the old game made people wait 5 seconds between planting cars
static constexpr unsigned int sliceCooldownMS = 1000;

/*
	1 byte		-	packet type
	12 bytes	-	selection box min corner, grid voxels, inclusive
	12 bytes	-	selection box max corner, exclusive

	Slices the bricks in the client's selection box into a vehicle, telling them why not if it can't, see sliceVehicle
*/
void sliceRequest(JoinedClient* source, Server const* const server, ENetPacket const* const packet, const void* pdv)
{
	const ServerProgramData* pd = (const ServerProgramData*)pdv;

	if (packet->dataLength < 1 + sizeof(int32_t) * 6)
		return;

	std::shared_ptr<ClientData> client = pd->getClient(source->me);
	if (!client)
		return;

	int32_t corners[6];
	memcpy(corners, packet->data + 1, sizeof(corners));

	glm::ivec3 low(corners[0], corners[1], corners[2]);
	glm::ivec3 high = glm::ivec3(corners[3], corners[4], corners[5]) - glm::ivec3(1);
	if (glm::any(glm::lessThan(high, low)))
		return;

	unsigned int now = SDL_GetTicks();
	if (now - client->lastSliceMS < sliceCooldownMS)
	{
		source->sendCenterPrint("Wait a moment before slicing again.", 2000, 1.0f, 0.4f, 0.4f);
		return;
	}
	client->lastSliceMS = now;

	std::string failure;
	std::shared_ptr<Vehicle> vehicle = sliceVehicle(client.get(), low, high, failure);
	if (!vehicle)
	{
		if (!failure.empty())
			source->sendCenterPrint(failure, 4000, 1.0f, 0.4f, 0.4f);
		return;
	}

	source->sendCenterPrint("Right click the vehicle to drive it.", 3000, 1.0f, 1.0f, 1.0f);
}
