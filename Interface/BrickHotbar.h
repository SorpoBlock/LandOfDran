#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"
#include "../Graphics/Texture.h"
#include "../Utility/SettingManager.h"

#include <functional>

//A basic brick size picked in the brick selector and held in a hot bar slot
struct HotbarBrick
{
	int width = 1;
	int height = 1;
	int length = 1;
	std::string name = "";

	//Not owned, one of the brick selector's icons
	Texture* icon = nullptr;
};

/*
	The row of brick slots along the bottom of the screen, like the old game's brick bar
	Number keys pick a slot and raise the bar, which is building mode: the ghost brick shows while a filled slot is picked
	Pressing the picked slot's key again, or the put away key, lowers the bar and ends building mode
	Never counts as an open window, it only draws over the game
*/
class BrickHotbar : public Window
{
	public:

	static constexpr int slotCount = 10;

	private:

	HotbarBrick slots[slotCount];
	bool filled[slotCount] = {};

	bool up = false;
	int selected = -1;
	bool changed = false;

	//Slide animation state, see render
	bool drawnUp = false;
	unsigned int slideStartMS = 0;

	public:

	//Also show the bar while it's lowered, e.g. while the brick selector is open so slots can be seen filling
	bool peek = false;

	//Like the old game: fills the first empty slot, or if every slot is full shifts them all along and replaces the first
	void add(const HotbarBrick& brick);

	//A number key press, slot 0 to slotCount - 1
	void pressSlot(int slot);

	//Mouse wheel, moves the picked slot by amount, wrapping around, only while the bar is up
	void scroll(int amount);

	//Slots are kept in settings under hotbar/ so they're still there next launch
	void save(std::shared_ptr<SettingManager> settings) const;
	void load(std::shared_ptr<SettingManager> settings, const std::function<Texture*(const std::string&)>& findIcon);

	void putAway();

	//The bar is up with a filled slot picked
	bool isBuilding() const;

	//The picked slot's brick, false if not building
	bool getSelected(HotbarBrick& brick) const;

	//True once after building mode starts or ends, or the picked brick changes
	bool takeChange();

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	BrickHotbar();
};
