#pragma once

#include "../LandOfDran.h"
#include "HeldServerPacket.h"
#include "PacketsFromServer/AcceptConnection.h"
#include "PacketsFromServer/AddSimObjectType.h"
#include "PacketsFromServer/AddSimObjects.h"
#include "PacketsFromServer/UpdateSimObjects.h"
#include "PacketsFromServer/DeleteSimObjects.h"
#include "PacketsFromServer/ChatMessageFromServer.h"
#include "PacketsFromServer/EvalLoginResponse.h"
#include "PacketsFromServer/DisplayConsoleLine.h"
#include "../GameLoop/ClientProgramData.h"
#include "../GameLoop/Simulation.h"

/*
	This is the networking interface that the client program should have one instance of, that managers sending and receiving from the server
	Do not confuse with JoinedClient, which is something the server may manage one or more of
	Also don't conufse with LoopClient which is just a container for the main game loop that the client program (as oppsed to dedicated servers) runs
*/
class Client
{
	ENetHost* client = nullptr;
	ENetPeer* peer = nullptr;

	//How long to hold server packets for if they can't be applied instantly
	unsigned int packetHoldTime = 10000;

	bool valid = false;

	std::vector<HeldServerPacket*> packets;

	//Try applying each held packet and deleting any that expired or have been applied
	void tryApplyHeldPackets(const ClientProgramData& pd, Simulation& simulation, const ExecutableArguments& cmdArgs);

public:

	float getPing() const		{ return (float)peer->lastRoundTripTime; }
	float getIncoming() const	{ return (float)peer->incomingBandwidth; }
	float getOutgoing() const	{ return (float)peer->outgoingBandwidth; }

	bool isValid() const { return valid; }

	//Returns true if we were kicked
	bool run(const ClientProgramData & pd,Simulation &simulation, const ExecutableArguments &cmdArgs);

	void send(const char* data, unsigned int len, PacketChannel channel);
	void send(ENetPacket* packet, PacketChannel channel);

	Client(std::string ip,unsigned int port,unsigned int _packetHoldTime);
	~Client();
};
