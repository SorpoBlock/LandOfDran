#include "ItemHotbar.h"

static constexpr float slideMS = 150.0f;
static constexpr float slotSize = 64.0f;
static constexpr float slotGap = 6.0f;
static constexpr float padding = 8.0f;
static constexpr float screenMargin = 10.0f;

static const ImU32 highlightColor = IM_COL32(255, 190, 0, 255);
static const ImU32 panelColor = IM_COL32(20, 20, 25, 190);

void ItemHotbar::toggle()
{
	up = !up;
	changed = true;
}

void ItemHotbar::putAway()
{
	if (!up)
		return;

	up = false;
	changed = true;
}

bool ItemHotbar::scroll(int amount)
{
	if (!up)
		return false;

	//Wheel up moves up the column
	if (amount != 0)
	{
		selected = ((selected - amount) % slotCount + slotCount) % slotCount;
		changed = true;
	}

	return true;
}

void ItemHotbar::setSlot(int slot, bool filled, const std::string& name, Texture* icon)
{
	if (slot < 0 || slot >= slotCount)
		return;

	slots[slot].filled = filled;
	slots[slot].name = name;
	slots[slot].icon = icon;
}

bool ItemHotbar::takeChange()
{
	bool result = changed;
	changed = false;
	return result;
}

void ItemHotbar::render(ImGuiIO* io)
{
	unsigned int now = SDL_GetTicks();

	//Restart the slide from wherever the last one got to, so quick taps don't make the bar jump
	if (up != drawnUp)
	{
		float progress = std::clamp((now - slideStartMS) / slideMS, 0.0f, 1.0f);
		slideStartMS = now - (unsigned int)((1.0f - progress) * slideMS);
		drawnUp = up;
	}

	float progress = std::clamp((now - slideStartMS) / slideMS, 0.0f, 1.0f);
	float shown = drawnUp ? progress : 1.0f - progress;
	if (shown <= 0.0f)
		return;

	//Ease out
	shown = 1.0f - (1.0f - shown) * (1.0f - shown);

	float titleHeight = ImGui::GetFontSize() + 6.0f;
	float width = slotSize + padding * 2.0f;
	float height = titleHeight + slotCount * slotSize + (slotCount - 1) * slotGap + padding;
	float left = io->DisplaySize.x - (width + screenMargin) * shown;
	float top = floor((io->DisplaySize.y - height) / 2.0f);

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	draw->AddRectFilled(ImVec2(left, top), ImVec2(left + width, top + height), panelColor, 6.0f);

	const char* title = "Items";
	ImVec2 titleSize = ImGui::CalcTextSize(title);
	draw->AddText(ImVec2(left + (width - titleSize.x) / 2.0f, top + 3.0f), IM_COL32_WHITE, title);

	for (int a = 0; a < slotCount; a++)
	{
		ImVec2 min(left + padding, top + titleHeight + a * (slotSize + slotGap));
		ImVec2 max(min.x + slotSize, min.y + slotSize);

		draw->AddRectFilled(min, max, IM_COL32(60, 60, 70, 200), 4.0f);

		if (slots[a].filled)
		{
			if (slots[a].icon && slots[a].icon->isValid())
				draw->AddImage((ImTextureID)(intptr_t)slots[a].icon->getHandle(), ImVec2(min.x + 4.0f, min.y + 4.0f), ImVec2(max.x - 4.0f, max.y - 4.0f));
			else
			{
				//No icon, its name wrapped inside the slot instead
				float lineHeight = ImGui::GetFontSize();
				draw->AddText(nullptr, 0.0f, ImVec2(min.x + 4.0f, min.y + 4.0f + lineHeight), IM_COL32_WHITE, slots[a].name.c_str(), nullptr, slotSize - 8.0f);
			}
		}

		std::string number = std::to_string(a + 1);
		draw->AddText(ImVec2(min.x + 4.0f, min.y + 2.0f), highlightColor, number.c_str());

		if (a == selected)
			draw->AddRect(min, max, highlightColor, 4.0f, 0, 3.0f);
		else
			draw->AddRect(min, max, IM_COL32(110, 110, 120, 200), 4.0f);
	}

	//The picked item's name beside its slot
	std::string label = slots[selected].filled ? slots[selected].name : "Empty";
	ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
	float slotTop = top + titleHeight + selected * (slotSize + slotGap);
	ImVec2 labelMax(left - 6.0f, slotTop + (slotSize + labelSize.y) / 2.0f + 4.0f);
	ImVec2 labelMin(labelMax.x - labelSize.x - 12.0f, labelMax.y - labelSize.y - 8.0f);
	draw->AddRectFilled(labelMin, labelMax, panelColor, 4.0f);
	draw->AddText(ImVec2(labelMin.x + 6.0f, labelMin.y + 4.0f), slots[selected].filled ? IM_COL32_WHITE : IM_COL32(170, 170, 180, 255), label.c_str());
}

void ItemHotbar::init()
{
	initalized = true;
}

void ItemHotbar::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{
}

ItemHotbar::ItemHotbar()
{
	name = "Item Hot Bar";
}
