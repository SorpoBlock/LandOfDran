#include "BrickSelector.h"

static constexpr int paletteColumns = 8;
static constexpr float iconSize = 64.0f;
static constexpr float cellWidth = 84.0f;

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

	unknownIcon = textures->createTexture("Assets/brick/types/Unknown.png");

	auto loadIcon = [this](const std::string& path)
	{
		Texture* icon = path.empty() ? nullptr : textures->createTexture(path);
		return icon && icon->isValid() ? icon : unknownIcon;
	};

	for (const BasicBrickType& type : types->getBasicTypes())
		icons.push_back(loadIcon(type.iconPath));

	for (size_t a = 0; a < types->getSpecialCount(); a++)
		specialIcons.push_back(loadIcon(types->getSpecial((int)a)->iconPath));
}

void BrickSelector::pick(int width, int height, int length, const std::string& brickName, Texture* icon, bool special)
{
	HotbarBrick brick;
	brick.width = width;
	brick.height = height;
	brick.length = length;
	brick.name = brickName;
	brick.icon = icon;
	brick.special = special;
	picks.push_back(brick);
}

bool BrickSelector::popPick(HotbarBrick& brick)
{
	if (picks.empty())
		return false;

	brick = picks.front();
	picks.erase(picks.begin());
	return true;
}

glm::u8vec4 BrickSelector::getColor() const
{
	return glm::u8vec4(glm::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f);
}

bool BrickSelector::takeColorChanged()
{
	bool result = colorChanged;
	colorChanged = false;
	return result;
}

void BrickSelector::save(std::shared_ptr<SettingManager> settings) const
{
	settings->addColor("hotbar/color", color);
	settings->addString("hotbar/material", brickMaterialNames[material]);
}

void BrickSelector::load(std::shared_ptr<SettingManager> settings)
{
	//Defaults to white when it's never been saved
	color = glm::clamp(settings->getColor("hotbar/color"), 0.0f, 1.0f);

	//None when it's never been saved
	material = std::max(findBrickMaterial(settings->getString("hotbar/material")), 0);
}

Texture* BrickSelector::findIcon(const std::string& brickName)
{
	if (!iconsLoaded)
		loadIcons();

	//Ignoring case, since settings files from before values kept their case have hot bar names lower cased
	const std::vector<BasicBrickType>& basicTypes = types->getBasicTypes();
	for (size_t a = 0; a < basicTypes.size() && a < icons.size(); a++)
	{
		if (lowercase(basicTypes[a].uiName) == lowercase(brickName))
			return icons[a];
	}

	int special = types->findSpecial(brickName);
	if (special >= 0 && special < (int)specialIcons.size())
		return specialIcons[special];

	return unknownIcon;
}

bool BrickSelector::iconButton(int id, Texture* icon, const std::string& label, int& column, int columns)
{
	if (column > 0)
		ImGui::SameLine(0, cellWidth - (iconSize + ImGui::GetStyle().FramePadding.x * 2.0f));

	ImGui::PushID(id);
	ImGui::BeginGroup();

	bool clicked;
	if (icon)
		clicked = ImGui::ImageButton("##icon", (ImTextureID)(intptr_t)icon->getHandle(), ImVec2(iconSize, iconSize));
	else
		clicked = ImGui::Button("?", ImVec2(iconSize + 8, iconSize + 8));

	//Special names like "45° Crest Corner" are wider than an icon
	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cellWidth - 4.0f);
	ImGui::TextUnformatted(label.c_str());
	ImGui::PopTextWrapPos();

	ImGui::EndGroup();
	ImGui::PopID();

	column = (column + 1) % columns;
	return clicked;
}

void BrickSelector::renderBasic()
{
	ImGui::TextUnformatted("Any size: width, height in plates, length");
	ImGui::InputInt3("##customSize", customSize);
	for (int& dimension : customSize)
		dimension = std::clamp(dimension, 1, 255);
	ImGui::SameLine();
	if (ImGui::Button("Add size"))
	{
		std::string sizeName = std::to_string(customSize[0]) + "x" + std::to_string(customSize[2]) + ", " + std::to_string(customSize[1]) + " plates tall";
		pick(customSize[0], customSize[1], customSize[2], sizeName, unknownIcon);
	}

	ImGui::Separator();

	ImGui::TextWrapped("%s", "Click a brick to add it to the hot bar, then press its number key to build with it");
	ImGui::BeginChild("brickTypes");

	const std::vector<BasicBrickType>& basicTypes = types->getBasicTypes();
	int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellWidth));
	int column = 0;

	for (size_t a = 0; a < basicTypes.size(); a++)
	{
		const BasicBrickType& type = basicTypes[a];
		if (iconButton((int)a, icons[a], type.uiName, column, columns))
			pick(type.width, type.height, type.length, type.uiName, icons[a]);
	}

	ImGui::EndChild();
}

void BrickSelector::renderSpecial()
{
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##specialFilter", "Search special bricks", specialFilter, sizeof(specialFilter));
	std::string filter = lowercase(specialFilter);

	ImGui::BeginChild("specialTypes");

	int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellWidth));

	//Types are sorted by category and then sub category, so each heading is a run of them
	size_t count = types->getSpecialCount();
	size_t start = 0;
	while (start < count)
	{
		const std::string& category = types->getSpecial((int)start)->category;
		size_t end = start;
		while (end < count && types->getSpecial((int)end)->category == category)
			end++;

		std::vector<size_t> shown;
		for (size_t a = start; a < end; a++)
		{
			const SpecialBrickType* type = types->getSpecial((int)a);
			if (!type->listed)
				continue;
			if (filter.empty() || lowercase(type->uiName).find(filter) != std::string::npos ||
				lowercase(type->category + " " + type->subCategory).find(filter) != std::string::npos)
				shown.push_back(a);
		}

		start = end;
		if (shown.empty())
			continue;

		//Open while searching so matches can be seen
		ImGui::SetNextItemOpen(true, filter.empty() ? ImGuiCond_Once : ImGuiCond_Always);
		std::string heading = category + " (" + std::to_string(shown.size()) + ")###" + category;
		if (!ImGui::CollapsingHeader(heading.c_str()))
			continue;

		std::string subCategory = "\x01";
		int column = 0;
		for (size_t a : shown)
		{
			const SpecialBrickType* type = types->getSpecial((int)a);
			if (type->subCategory != subCategory)
			{
				subCategory = type->subCategory;
				column = 0;
				if (!subCategory.empty())
					ImGui::SeparatorText(subCategory.c_str());
			}

			if (iconButton((int)(a + 100000), specialIcons[a], type->uiName, column, columns))
				pick(type->width, type->height, type->length, type->uiName, specialIcons[a], true);
		}
	}

	ImGui::EndChild();
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
			colorChanged = true;
		}
		ImGui::PopID();

		if ((a + 1) % paletteColumns != 0)
			ImGui::SameLine();
	}

	//Only counts as changed once an edit finishes, not every frame of a drag, since changes get saved to file
	ImGui::ColorEdit4("Custom color", &color[0], ImGuiColorEditFlags_AlphaBar);
	if (ImGui::IsItemDeactivatedAfterEdit())
		colorChanged = true;

	//Painted on along with the color
	if (ImGui::Combo("Material", &material, brickMaterialNames, BrickMaterialCount))
		colorChanged = true;

	ImGui::Separator();

	if (ImGui::BeginTabBar("brickKinds"))
	{
		if (ImGui::BeginTabItem("Basic"))
		{
			renderBasic();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Special"))
		{
			renderSpecial();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

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
	std::unordered_set<Texture*> released;
	for (Texture* icon : icons)
	{
		if (icon && released.insert(icon).second)
			icon->markForCleanup();
	}
	for (Texture* icon : specialIcons)
	{
		if (icon && released.insert(icon).second)
			icon->markForCleanup();
	}
}
