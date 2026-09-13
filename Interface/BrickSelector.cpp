#include "BrickSelector.h"

static constexpr int paletteColumns = 8;

//Same count as the old game's palette, the last row is transparent
static const glm::vec4 palette[] =
{
	{ 1, 1, 1, 1 }, { 0.9, 0.9, 0.9, 1 }, { 0.75, 0.75, 0.75, 1 }, { 0.5, 0.5, 0.5, 1 }, { 0.3, 0.3, 0.3, 1 }, { 0.15, 0.15, 0.15, 1 }, { 0.05, 0.05, 0.05, 1 }, { 0.6, 0.55, 0.5, 1 },
	{ 0.9, 0, 0, 1 }, { 0.6, 0, 0, 1 }, { 1, 0.4, 0.4, 1 }, { 0.95, 0.45, 0, 1 }, { 1, 0.7, 0.3, 1 }, { 0.9, 0.9, 0, 1 }, { 1, 1, 0.5, 1 }, { 0.75, 0.6, 0.1, 1 },
	{ 0, 0.5, 0.25, 1 }, { 0, 0.75, 0, 1 }, { 0.4, 0.9, 0.4, 1 }, { 0.2, 0.35, 0.1, 1 }, { 0.55, 0.7, 0.2, 1 }, { 0, 0.6, 0.6, 1 }, { 0.4, 0.9, 0.85, 1 }, { 0.1, 0.3, 0.3, 1 },
	{ 0.2, 0, 0.8, 1 }, { 0, 0.3, 0.8, 1 }, { 0.4, 0.6, 1, 1 }, { 0, 0.1, 0.35, 1 }, { 0.5, 0.3, 0.8, 1 }, { 0.75, 0.5, 1, 1 }, { 0.8, 0.1, 0.6, 1 }, { 1, 0.6, 0.85, 1 },
	{ 0.4, 0.25, 0.1, 1 }, { 0.6, 0.4, 0.2, 1 }, { 0.8, 0.65, 0.45, 1 }, { 0.25, 0.15, 0.05, 1 }, { 1, 1, 1, 0.4 }, { 0.6, 0.8, 1, 0.4 }, { 1, 0.3, 0.3, 0.4 }, { 0.3, 1, 0.3, 0.4 }
};

void BrickSelector::loadIcons()
{
	iconsLoaded = true;

	Texture* unknown = textures->createTexture("Assets/brick/types/Unknown.png");

	for (const BasicBrickType& type : types->getBasicTypes())
	{
		Texture* icon = type.iconPath.empty() ? nullptr : textures->createTexture(type.iconPath);
		icons.push_back(icon && icon->isValid() ? icon : unknown);
	}
}

void BrickSelector::choose(int width, int height, int length)
{
	selectedSize[0] = width;
	selectedSize[1] = height;
	selectedSize[2] = length;
	selectionWaiting = true;
}

void BrickSelector::getSelection(int& width, int& height, int& length, glm::u8vec4& brickColor)
{
	selectionWaiting = false;
	width = selectedSize[0];
	height = selectedSize[1];
	length = selectedSize[2];
	brickColor = glm::u8vec4(glm::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f);
}

void BrickSelector::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if (!iconsLoaded)
		loadIcons();

	ImGui::SetNextWindowSize(ImVec2(560, 560), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Brick Selector", &opened))
	{
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Color");
	for (int a = 0; a < (int)(sizeof(palette) / sizeof(palette[0])); a++)
	{
		ImGui::PushID(a);
		ImVec4 swatch(palette[a].r, palette[a].g, palette[a].b, palette[a].a);
		if (ImGui::ColorButton("##palette", swatch, ImGuiColorEditFlags_AlphaPreview, ImVec2(24, 24)))
		{
			color = palette[a];
			choose(selectedSize[0], selectedSize[1], selectedSize[2]);
		}
		ImGui::PopID();

		if ((a + 1) % paletteColumns != 0)
			ImGui::SameLine();
	}

	if (ImGui::ColorEdit4("Custom color", &color[0], ImGuiColorEditFlags_AlphaBar))
		choose(selectedSize[0], selectedSize[1], selectedSize[2]);

	ImGui::Separator();

	ImGui::TextUnformatted("Any size: width, height in plates, length");
	ImGui::InputInt3("##customSize", customSize);
	for (int& dimension : customSize)
		dimension = std::clamp(dimension, 1, 255);
	ImGui::SameLine();
	if (ImGui::Button("Use size"))
		choose(customSize[0], customSize[1], customSize[2]);

	ImGui::Separator();

	ImGui::BeginChild("brickTypes");

	const std::vector<BasicBrickType>& basicTypes = types->getBasicTypes();
	const float cellWidth = 84.0f;
	int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellWidth));

	for (size_t a = 0; a < basicTypes.size(); a++)
	{
		const BasicBrickType& type = basicTypes[a];

		ImGui::PushID((int)a);
		ImGui::BeginGroup();

		bool clicked;
		if (icons[a])
			clicked = ImGui::ImageButton("##icon", (ImTextureID)(intptr_t)icons[a]->getHandle(), ImVec2(64, 64));
		else
			clicked = ImGui::Button("?", ImVec2(72, 72));

		ImGui::TextUnformatted(type.uiName.c_str());
		ImGui::EndGroup();

		if (clicked)
			choose(type.width, type.height, type.length);

		ImGui::PopID();

		if ((a + 1) % columns != 0)
			ImGui::SameLine(0, cellWidth - 72);
	}

	ImGui::EndChild();
	ImGui::End();
}

void BrickSelector::init()
{
	initalized = true;
}

void BrickSelector::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{
}

BrickSelector::BrickSelector(const BrickTypes* _types, std::shared_ptr<TextureManager> _textures) : types(_types), textures(_textures)
{
	name = "Brick Selector";
}

BrickSelector::~BrickSelector()
{
	for (Texture* icon : icons)
	{
		if (icon)
			icon->markForCleanup();
	}
}
