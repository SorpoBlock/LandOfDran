#include "VehicleLoader.h"
#include "../Bricks/BrickSaves.h"

void VehicleLoader::refresh()
{
	std::string previous = selected >= 0 && selected < (int)names.size() ? names[selected] : "";

	names.clear();
	selected = -1;

	std::error_code errorCode;
	if (std::filesystem::is_directory("Saves/Vehicles", errorCode))
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("Saves/Vehicles", errorCode))
		{
			if (entry.is_regular_file() && lowercase(entry.path().extension().string()) == ".lod")
				names.push_back(entry.path().stem().string());
		}
	}

	std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) { return lowercase(a) < lowercase(b); });

	for (size_t a = 0; a < names.size(); a++)
	{
		if (names[a] == previous)
			selected = (int)a;
	}
}

void VehicleLoader::openLoader()
{
	refresh();
	open();
}

bool VehicleLoader::takeRequest(std::string& path, bool& asVehicle)
{
	if (!requested)
		return false;

	requested = false;
	if (selected < 0 || selected >= (int)names.size())
		return false;

	path = getVehicleSavePath(names[selected]);
	asVehicle = requestAsVehicle;
	return !path.empty();
}

void VehicleLoader::render(ImGuiIO* io)
{
	if (!opened)
		return;

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::Begin("Saved Vehicles", &opened, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End();
		return;
	}

	float width = ImGui::GetFontSize() * 20.0f;

	if (names.empty())
	{
		ImGui::PushTextWrapPos(width);
		ImGui::TextDisabled("No saved vehicles yet. Wrench a vehicle to save it.");
		ImGui::PopTextWrapPos();
	}
	else if (ImGui::BeginListBox("##Vehicles", ImVec2(width, ImGui::GetTextLineHeightWithSpacing() * std::clamp((float)names.size(), 4.0f, 12.0f))))
	{
		for (size_t a = 0; a < names.size(); a++)
		{
			ImGui::PushID((int)a);
			if (ImGui::Selectable(names[a].c_str(), selected == (int)a))
				selected = (int)a;
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}

	ImGui::TextDisabled("Shows it at your crosshair, left click places it");

	ImGui::BeginDisabled(selected < 0);
	if (ImGui::Button("Load as vehicle"))
	{
		requested = true;
		requestAsVehicle = true;
		close();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Ready to drive, right click it to get in");

	ImGui::SameLine();

	if (ImGui::Button("Load as bricks"))
	{
		requested = true;
		requestAsVehicle = false;
		close();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Plain bricks you can change and slice into a vehicle again, Ctrl+Z undoes them");
	ImGui::EndDisabled();

	ImGui::SameLine();

	if (ImGui::Button("Refresh"))
		refresh();

	ImGui::End();
}

void VehicleLoader::init()
{

}

void VehicleLoader::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{

}

VehicleLoader::VehicleLoader()
{
	name = "Saved Vehicles";
}

VehicleLoader::~VehicleLoader()
{

}
