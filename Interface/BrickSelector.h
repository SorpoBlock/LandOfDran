#pragma once

#include "../LandOfDran.h"
#include "UserInterface.h"
#include "../Bricks/BrickTypes.h"
#include "../Graphics/Texture.h"

//Picks the size and color of the ghost brick
class BrickSelector : public Window
{
	const BrickTypes* types = nullptr;
	std::shared_ptr<TextureManager> textures = nullptr;

	//One per basic type, loaded the first time the window opens
	std::vector<Texture*> icons;
	bool iconsLoaded = false;

	int customSize[3] = { 2, 3, 4 };
	glm::vec4 color = glm::vec4(1, 1, 1, 1);

	int selectedSize[3] = { 2, 3, 4 };
	bool selectionWaiting = false;

	void loadIcons();
	void choose(int width, int height, int length);

	public:

	bool hasSelection() const { return selectionWaiting; }

	//Resets hasSelection
	void getSelection(int& width, int& height, int& length, glm::u8vec4& brickColor);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;
	virtual void handleInput(SDL_Event& e, std::shared_ptr<InputMap> input) override;

	BrickSelector(const BrickTypes* _types, std::shared_ptr<TextureManager> _textures);
	~BrickSelector();
};
