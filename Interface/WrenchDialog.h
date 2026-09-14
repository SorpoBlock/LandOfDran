#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"
#include "../Bricks/BrickAttachments.h"

//A brick's settings as the wrench dialog edits them, sent back to the server in a WrenchSubmit packet
struct WrenchSubmission
{
	netIDType brickID = NO_ID;
	bool collides = true;
	std::string name = "";
	BrickAttachments attachments;
};

/*
	Changes a brick's collision, name, music loop, light, and emitter
	The server opens it when a player wrenches a brick, or when Lua calls client:openWrenchDialog, see OpenWrenchDialogPacket
*/
class WrenchDialog : public Window
{
	WrenchSubmission editing;

	//What kind of brick it is, shown at the top
	std::string brickLabel = "";

	//What the server has to pick from, plus the brick's own pick if Lua gave it one that isn't listed
	std::vector<std::string> musicNames;
	std::vector<std::string> emitterNames;

	//The spotlight's direction as sliders, in degrees, a pitch of -90 points straight down
	float lightYaw = 0;
	float lightPitch = -90;

	//What checking Spotlight again brings the cone angle back to
	float lastConeAngle = 60;

	bool submitted = false;

	//Focused the frame after it opens, and centered for a few frames, since its size is only known once the settings have been measured
	bool justOpened = false;
	int framesToCenter = 0;

	public:

	//Shows a brick's settings from the server, replacing anything that was being edited
	void openFor(const WrenchSubmission& settings, const std::string& label, const std::vector<std::string>& music, const std::vector<std::string>& emitters);

	//True once after Apply is clicked, with what to send
	bool takeSubmission(WrenchSubmission& submission);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	WrenchDialog();
	~WrenchDialog();
};
