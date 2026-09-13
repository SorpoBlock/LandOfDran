#pragma once

#include "../LandOfDran.h"

#include "../Graphics/ShaderSpecification.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/RenderContext.h"
#include "../Interface/InputMap.h"
#include "../Interface/SettingsMenu.h"
#include "../Interface/DebugMenu.h"
#include "../Interface/ServerBrowser.h"
#include "../Interface/EscapeMenu.h"
#include "../Physics/PhysicsWorld.h"
#include "../Interface/ChatWindow.h"
#include "../Graphics/RenderTarget.h"
#include "../Graphics/Environment.h"
#include "../Graphics/InstancedBrickRenderer.h"
#include "../Bricks/GhostBrick.h"
#include "../Bricks/BrickTypes.h"
#include "../Interface/BrickSelector.h"
#include "../Interface/BrickHotbar.h"
#include "../Audio/AudioSystem.h"

/*
	This exists so we can make all of this available to the various PacketsFromServer files since packets can do a wide range of activities
	and may need access to most of the game state 
	It's owned by LoopClient and passed through Client to various packet processing methods
*/
struct ClientProgramData
{
	Material* grassMaterial = nullptr;
	GLuint grassVao = 0;

	//TODO: Move this to environment class and don't hardcode it to 3
	glm::mat4 lightSpaceMatricies[3];
	GLuint lightSpaceMatriciesUniformShadow = 0;
	GLuint lightSpaceMatriciesUniformModel = 0;
	GLuint lightSpaceMatriciesUniformBrick = 0;
	GLuint lightSpaceMatriciesUniformBrickShadow = 0;

	//Lives for the whole program, bricks of whichever server we're on are passed to it by Simulation's BrickHolder
	InstancedBrickRenderer* brickRenderer = nullptr;

	//Named brick sizes with icons, for the brick selector
	BrickTypes brickTypes;

	GhostBrick ghostBrick;
	std::shared_ptr<BrickSelector> brickSelector = nullptr;
	std::shared_ptr<BrickHotbar> brickHotbar = nullptr;

	/*
		Things the game remembers between launches rather than settings the player picks: last server and name,
		window size, hot bar. Kept out of Config/settings.txt so they can be written often without rewriting that
	*/
	std::shared_ptr<SettingManager> state = nullptr;
	static constexpr const char* stateFilePath = "Config/state.txt";

	//graphics/startresolutionx and y as of launch or the last settings save, so a newly picked one can be applied
	glm::ivec2 appliedStartResolution = glm::ivec2(0);
	std::shared_ptr<RenderTarget> shadows = nullptr;

	Environment environment;

	//Empty, sky.vert builds a fullscreen triangle from gl_VertexID but core profile still needs a VAO bound
	GLuint skyVao = 0;

	GLuint waterVao = 0;
	GLuint waterVbo = 0;
	GLsizei waterVertexCount = 0;

	//The scene above and below the water surface, both nullptr when graphics/waterquality is off
	std::shared_ptr<RenderTarget> waterReflection = nullptr;
	std::shared_ptr<RenderTarget> waterRefraction = nullptr;

	std::shared_ptr<RenderContext>	context = nullptr;
	std::shared_ptr<UserInterface>	gui = nullptr;
	std::shared_ptr<SettingsMenu>	settingsMenu = nullptr;
	std::shared_ptr<DebugMenu>		debugMenu = nullptr;
	std::shared_ptr<ServerBrowser>	serverBrowser = nullptr;
	std::shared_ptr<ShaderManager>	shaders = nullptr;
	std::shared_ptr<TextureManager> textures = nullptr;
	std::shared_ptr<InputMap>		input = nullptr;
	std::shared_ptr<EscapeMenu>		escapeMenu = nullptr;
	std::shared_ptr<ChatWindow>		chatWindow = nullptr;

	//Lives for the whole program, sound types come from whichever server we're on
	std::shared_ptr<AudioSystem>	audio = nullptr;

	//TODO: Move to simulation
	std::shared_ptr<PhysicsWorld>	physicsWorld = nullptr;

	//The rest of this struct is passed as const to various PacketsFromServer functions
	//If a packet wants to communicate to the main loop it should do so here
	struct PacketSignals
	{
		//These two are set by AcceptConnection.cpp
		bool startPhaseOneLoading = false;
		unsigned int typesToLoad = 0;
		//Set by AddSimObjectType
		bool finishedPhaseOneLoading = false;

		//Call this after each frame
		void reset()
		{
			finishedPhaseOneLoading = false;
			startPhaseOneLoading = false;
		}
	} signals;

	//You should be able to edit the signals from received packets function bodies, just not the pointers to essential software systems
	PacketSignals * getSignals() const { return const_cast<PacketSignals*>(&signals); }
};
