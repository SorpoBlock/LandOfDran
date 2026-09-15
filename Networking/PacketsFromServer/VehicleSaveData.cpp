#include "VehicleSaveData.h"

/*
	1 byte		-	packet type
	4 bytes		-	vehicle net ID
	4 bytes		-	size of the whole file
	4 bytes		-	where in the file this packet's bytes go
	The rest	-	file bytes

	Part of a vehicle save we asked for from its wrench dialog, written to Saves/Vehicles once all of it arrives
*/
bool VehicleSaveDataPacket::applyPacket(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs)
{
	static constexpr unsigned int headerBytes = 1 + sizeof(netIDType) + sizeof(uint32_t) * 2;

	if (cmdArgs.gameState != InGame)
		return false;

	if (packet->dataLength < headerBytes)
		return true;

	netIDType vehicleID;
	uint32_t total, offset;
	memcpy(&vehicleID, packet->data + 1, sizeof(netIDType));
	memcpy(&total, packet->data + 1 + sizeof(netIDType), sizeof(uint32_t));
	memcpy(&offset, packet->data + 1 + sizeof(netIDType) + sizeof(uint32_t), sizeof(uint32_t));

	//Not one we asked for
	auto found = simulation.vehicleSaves.find(vehicleID);
	if (found == simulation.vehicleSaves.end())
		return true;

	Simulation::PendingVehicleSave& save = found->second;
	if (offset == 0)
		save.bytes.clear();
	else if (offset != save.bytes.size())
		return true;

	save.bytes.append((const char*)packet->data + headerBytes, packet->dataLength - headerBytes);
	if (save.bytes.size() < total)
		return true;

	std::error_code errorCode;
	std::filesystem::create_directories("Saves/Vehicles", errorCode);

	std::ofstream file(save.path, std::ios::binary);
	bool written = file.is_open() && file.write(save.bytes.data(), save.bytes.size());
	file.close();

	if (written)
	{
		info("Saved a vehicle to " + save.path);
		pd.gui->addCenterPrint("Saved to " + save.path, 3000, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		error("Couldn't write " + save.path);
		pd.gui->addCenterPrint("Couldn't write " + save.path, 4000, 1.0f, 0.4f, 0.4f);
	}

	simulation.vehicleSaves.erase(found);
	pd.vehicleLoader->refresh();
	return true;
}

VehicleSaveDataPacket::VehicleSaveDataPacket(unsigned int holdTime, ENetPacket* _packet)
{
	packet = _packet;
	deletionTime = SDL_GetTicks() + holdTime;
}

VehicleSaveDataPacket::~VehicleSaveDataPacket()
{
	if (packet)
		enet_packet_destroy(packet);
}
