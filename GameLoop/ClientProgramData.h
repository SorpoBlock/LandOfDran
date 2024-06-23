#pragma once

#include "../LandOfDran.h"

#include "../Graphics/ShaderSpecification.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/RenderContext.h"
#include "../Interface/InputMap.h"
#include "../Graphics/PlayerCamera.h"
#include "../Interface/SettingsMenu.h"
#include "../Interface/DebugMenu.h"
#include "../Interface/ServerBrowser.h"
#include "../Interface/EscapeMenu.h"
#include "../Physics/PhysicsWorld.h"

/*
	This exists so we can make all of this available to the various PacketsFromServer files since packets can do a wide range of activities
	and may need access to most of the game state
	It's owned by LoopClient and passed through Client to various packet processing methods
*/
struct ClientProgramData
{
	std::shared_ptr<RenderContext>	context = nullptr;
	std::shared_ptr<UserInterface>	gui = nullptr;
	std::shared_ptr<SettingsMenu>	settingsMenu = nullptr;
	std::shared_ptr<DebugMenu>		debugMenu = nullptr;
	std::shared_ptr<ServerBrowser>	serverBrowser = nullptr;
	std::shared_ptr<ShaderManager>	shaders = nullptr;
	std::shared_ptr<Camera>			camera = nullptr;
	std::shared_ptr<TextureManager> textures = nullptr;
	std::shared_ptr<InputMap>		input = nullptr;
	std::shared_ptr<EscapeMenu>		escapeMenu = nullptr;
	std::shared_ptr<PhysicsWorld>	physicsWorld = nullptr;

	//The rest of this struct is passed as const to various PacketsFromServer functions
	//If a packet wants to communicate to the main loop it should do so here
	struct PacketSignals
	{
		//These two are set by AcceptConnection.cpp
		bool startPhaseOneLoading = false;
		unsigned int typesToLoad = 0;

		//Call this after each frame
		void reset()
		{
			startPhaseOneLoading = false;
		}
	} signals;

	//You should be able to edit the signals from received packets function bodies, just not the pointers to essential software systems
	PacketSignals * getSignals() const { return const_cast<PacketSignals*>(&signals); }
};
