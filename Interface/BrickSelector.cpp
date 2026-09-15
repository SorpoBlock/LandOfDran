#include "BrickSelector.h"

static constexpr float iconSize = 64.0f;
static constexpr float cellWidth = 84.0f;

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

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextWrapped("%s", "Click a brick to add it to the hot bar, then press its number key to build with it. E picks the paint color and material");
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

		//A blank line between categories, but not above the first
		if (ImGui::GetCursorPosY() > ImGui::GetCursorStartPos().y)
			ImGui::NewLine();

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
				//A blank line after the previous sub category's icons
				if (a != shown.front())
					ImGui::NewLine();

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
