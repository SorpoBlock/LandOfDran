#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"

/*
	Lists the vehicles saved to Saves/Vehicles from vehicles' wrench dialogs, opened from the escape menu,
	and picks one to place with VehicleGhost as a vehicle ready to drive or as plain bricks, like the old game's car loading
*/
class VehicleLoader : public Window
{
	//File names without .lod
	std::vector<std::string> names;
	int selected = -1;

	bool requested = false;
	bool requestAsVehicle = true;

	public:

	//Finds the saves again, keeping the same one picked if it's still there
	void refresh();

	//Refreshes and opens it
	void openLoader();

	//True once after a load button is clicked, with the file to upload and how to load it
	bool takeRequest(std::string& path, bool& asVehicle);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	VehicleLoader();
	~VehicleLoader();
};
