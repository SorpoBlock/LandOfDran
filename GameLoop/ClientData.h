#pragma once

#include "../Networking/JoinedClient.h"
#include "../SimObjects/Dynamic.h"
#include "../SimObjects/Light.h"
#include "PlayerController.h"
#include "PlayerAppearance.h"

#include <set>

struct ServerProgramData;

//Basically JoinedClient is lower level and used by the server for networking, ClientData is used in server-side packet functions
//ClientData contains references to a JoinedClient but also anything else that client 'owns' like a player, a camera, bricks, etc.
struct ClientData
{
	//Smart pointer to give to owned objects that want to refer to this object, managed by ServerProgramData methods
	std::shared_ptr<ClientData> me;

	//Lower level networking stuff
	std::shared_ptr<JoinedClient> client;

	//The client also keeps one of these client-side
	//Attaches to one or more dynamics like players etc, handles movement from movement keys
	std::vector<PlayerController> controllers;

	//Physics objects like the player that this client handles primary simulation of
	std::vector<std::shared_ptr<Dynamic>> controlledObjects;

	//IDs of bricks this client planted, newest last, for undo
	std::vector<netIDType> plantedBricks;

	//The brick in the last wrench dialog sent to them, the only one a WrenchSubmit from them can change, NO_ID once they've submitted
	netIDType wrenchedBrickID = NO_ID;

	//Voice chat, see Networking/PacketsFromClient/VoiceFrame.cpp
	//Lua's client:setVoiceMuted, their voice is dropped while it's set
	bool voiceMuted = false;
	//Between the ClientStartTalking and ClientStopTalking events
	bool talking = false;
	//SDL_GetTicks of their last voice packet, see LoopServer::endQuietTalkers
	unsigned int lastVoiceMS = 0;
	//Net IDs of clients whose names this client has been sent, so it can show who's talking
	std::set<netIDType> knownTalkers;

	//Lua's client:setJetsEnabled and client:setFlashlightEnabled, both allowed unless Lua says otherwise
	bool jetsEnabled = true;
	bool flashlightEnabled = true;

	//Their flashlight while it's on, held by the target of their first controller, see setFlashlight
	std::weak_ptr<Light> flashlight;

	//How they want their player to look, from their game as they connect, see Networking/PacketsFromClient/AppearanceChoice.cpp
	PlayerAppearance appearance;

	//The last dynamic Lua's client:applyAppearance put it on, which gets their changes if they save new ones while playing
	std::weak_ptr<Dynamic> appearanceTarget;

	//Turns their flashlight on in color (or just recolors it), or off, playing LightOn or LightOff from their player for anyone nearby
	//Won't turn it on while flashlightEnabled is off, or without a player from setDefaultController to hold it
	void setFlashlight(const ServerProgramData* pd, bool on, const glm::vec3& color);

	//Tells their game whether jets and the flashlight are enabled, so it doesn't jet on its own when the server won't
	void sendAbilities() const;

	//Removes their flashlight and jet flames, for when they leave
	void removeEffects(const ServerProgramData* pd);
};
