#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"
#include "BrickHotbar.h"
#include "../Bricks/Brick.h"
#include "../Bricks/BrickTypes.h"
#include "../Graphics/Texture.h"

//Picks brick sizes to put in the hot bar, and the color to build with
class BrickSelector : public Window
{
	const BrickTypes* types = nullptr;
	std::shared_ptr<TextureManager> textures = nullptr;

	//One per basic type and one per special type, loaded the first time the window opens
	std::vector<Texture*> icons;
	std::vector<Texture*> specialIcons;
	Texture* unknownIcon = nullptr;
	bool iconsLoaded = false;

	int customSize[3] = { 2, 3, 4 };
	glm::vec4 color = glm::vec4(1, 1, 1, 1);

	//A BrickMaterial, int for ImGui::Combo
	int material = BrickMaterial_None;

	//Only special bricks whose name or category contains this are listed
	char specialFilter[64] = "";

	//Bricks clicked since the last popPick, oldest first
	std::vector<HotbarBrick> picks;

	bool colorChanged = false;

	void loadIcons();
	void pick(int width, int height, int length, const std::string& brickName, Texture* icon, bool special = false);

	//One icon button with its name under it, true if clicked, wraps to a new row when the last one filled the row
	bool iconButton(int id, Texture* icon, const std::string& label, int& column, int columns);

	void renderBasic();
	void renderSpecial();

	public:

	//Takes the oldest brick clicked since the last call, false if there are none
	bool popPick(HotbarBrick& brick);

	glm::u8vec4 getColor() const;
	unsigned char getMaterial() const { return (unsigned char)material; }

	//True once after the building color or material is changed in the window
	bool takeColorChanged();

	//The building color and material are kept in settings as hotbar/color and hotbar/material
	void save(std::shared_ptr<SettingManager> settings) const;
	void load(std::shared_ptr<SettingManager> settings);

	//A basic or special type's icon by its name, or the unknown icon, loading icons first if needed
	Texture* findIcon(const std::string& brickName);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	BrickSelector(const BrickTypes* _types, std::shared_ptr<TextureManager> _textures);
	~BrickSelector();
};
