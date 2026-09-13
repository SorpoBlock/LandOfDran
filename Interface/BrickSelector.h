#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"
#include "BrickHotbar.h"
#include "../Bricks/BrickTypes.h"
#include "../Graphics/Texture.h"

//Picks brick sizes to put in the hot bar, and the color to build with
class BrickSelector : public Window
{
	const BrickTypes* types = nullptr;
	std::shared_ptr<TextureManager> textures = nullptr;

	//One per basic type, loaded the first time the window opens
	std::vector<Texture*> icons;
	Texture* unknownIcon = nullptr;
	bool iconsLoaded = false;

	int customSize[3] = { 2, 3, 4 };
	glm::vec4 color = glm::vec4(1, 1, 1, 1);

	//Bricks clicked since the last popPick, oldest first
	std::vector<HotbarBrick> picks;

	bool colorChanged = false;

	void loadIcons();
	void pick(int width, int height, int length, const std::string& brickName, Texture* icon);

	public:

	//Takes the oldest brick clicked since the last call, false if there are none
	bool popPick(HotbarBrick& brick);

	glm::u8vec4 getColor() const;

	//True once after the building color is changed in the window
	bool takeColorChanged();

	//The building color is kept in settings as hotbar/color
	void save(std::shared_ptr<SettingManager> settings) const;
	void load(std::shared_ptr<SettingManager> settings);

	//A basic type's icon by its name, or the unknown icon, loading icons first if needed
	Texture* findIcon(const std::string& brickName);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	BrickSelector(const BrickTypes* _types, std::shared_ptr<TextureManager> _textures);
	~BrickSelector();
};
