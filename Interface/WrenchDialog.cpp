#include "WrenchDialog.h"

#include <cmath>

//A dropdown with None first, for picking music or an emitter by name
static void nameCombo(const char* label, std::string& picked, const std::vector<std::string>& names)
{
	if (!ImGui::BeginCombo(label, picked.empty() ? "None" : picked.c_str()))
		return;

	if (ImGui::Selectable("None", picked.empty()))
		picked = "";

	for (size_t a = 0; a < names.size(); a++)
	{
		ImGui::PushID((int)a);
		if (ImGui::Selectable(names[a].c_str(), names[a] == picked))
			picked = names[a];
		ImGui::PopID();
	}

	ImGui::EndCombo();
}

static void tooltip(const char* text)
{
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", text);
}

void WrenchDialog::openFor(const WrenchSubmission& settings, const std::string& label, const std::vector<std::string>& music, const std::vector<std::string>& emitters)
{
	editing = settings;
	brickLabel = label;
	musicNames = music;
	emitterNames = emitters;

	//Applying without touching them keeps whatever Lua put on the brick
	const std::string& musicName = editing.attachments.musicName;
	if (!musicName.empty() && std::find(musicNames.begin(), musicNames.end(), musicName) == musicNames.end())
		musicNames.push_back(musicName);

	const std::string& emitterName = editing.attachments.emitterName;
	if (!emitterName.empty() && std::find(emitterNames.begin(), emitterNames.end(), emitterName) == emitterNames.end())
		emitterNames.push_back(emitterName);

	const glm::vec3& direction = editing.attachments.lightDirection;
	lightPitch = glm::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f)));
	lightYaw = glm::degrees(std::atan2(direction.x, direction.z));

	if (editing.attachments.lightConeAngle > 0.0f)
		lastConeAngle = editing.attachments.lightConeAngle;

	submitted = false;
	justOpened = true;
	framesToCenter = 3;
	open();
}

bool WrenchDialog::takeSubmission(WrenchSubmission& submission)
{
	if (!submitted)
		return false;

	submitted = false;
	submission = editing;
	submission.name = submission.name.substr(0, 255);
	submission.attachments.clampValues();
	return true;
}

void WrenchDialog::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if (framesToCenter > 0)
	{
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		framesToCenter--;
	}

	if (justOpened)
	{
		ImGui::SetNextWindowFocus();
		justOpened = false;
	}

	if (!ImGui::Begin("Wrench", &opened, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End();
		return;
	}

	const ImGuiStyle& style = ImGui::GetStyle();
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	//Opening sections grows it downward from where it opened, which can push it past the bottom of the screen
	ImVec2 position = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();
	float x = std::clamp(position.x, viewport->WorkPos.x, std::max(viewport->WorkPos.x, viewport->WorkPos.x + viewport->WorkSize.x - size.x));
	float y = std::clamp(position.y, viewport->WorkPos.y, std::max(viewport->WorkPos.y, viewport->WorkPos.y + viewport->WorkSize.y - size.y));
	if (x != position.x || y != position.y)
		ImGui::SetWindowPos(ImVec2(x, y));

	BrickAttachments& settings = editing.attachments;

	ImGui::TextUnformatted(brickLabel.c_str());
	ImGui::Separator();

	//The settings scroll once they'd be taller than most of the screen, which a light and large UI scaling easily make them, so Apply stays in view
	float itemWidth = ImGui::GetFontSize() * 16.0f;
	float bodyWidth = itemWidth + style.ItemInnerSpacing.x + ImGui::CalcTextSize("Brightness").x + style.ScrollbarSize + ImGui::GetFontSize();
	float chrome = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f + ImGui::GetTextLineHeightWithSpacing() * 2.0f + ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y * 2.0f;
	float maxBodyHeight = std::max(ImGui::GetFontSize() * 6.0f, viewport->WorkSize.y * 0.95f - chrome);
	ImGui::SetNextWindowSizeConstraints(ImVec2(bodyWidth, 0.0f), ImVec2(bodyWidth, maxBodyHeight));
	ImGui::BeginChild("Settings", ImVec2(bodyWidth, 0.0f), ImGuiChildFlags_AutoResizeY);
	ImGui::PushItemWidth(itemWidth);

	ImGui::Checkbox("Colliding", &editing.collides);
	tooltip("Whether players and objects bump into it. Clicks and raycasts hit it either way");

	ImGui::InputText("Name", &editing.name);
	tooltip("For scripts to find it by");

	if (ImGui::CollapsingHeader("Music", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (musicNames.empty())
			ImGui::TextDisabled("The server has no music");
		else
		{
			nameCombo("Loop", settings.musicName, musicNames);

			if (!settings.musicName.empty())
			{
				ImGui::SliderFloat("Volume", &settings.musicVolume, 0.0f, 1.0f, "%.2f");
				ImGui::SliderFloat("Pitch##Music", &settings.musicPitch, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
				tooltip("Playback speed. Ctrl+click to type anything from 0.05 to 10");
			}
		}
	}

	if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Has light", &settings.hasLight);

		if (settings.hasLight)
		{
			ImGui::ColorEdit3("Color", &settings.lightColor[0]);

			ImGui::SliderFloat("Brightness", &settings.lightBrightness, 0.0f, 100000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
			tooltip("A lamp is around 20 to 100, a floodlight a few thousand. How far it reaches follows from this");

			ImGui::SliderFloat("Flicker", &settings.lightFlicker, 0.0f, 16.0f, "%.2f studs");
			tooltip("How far the light jumps around, like a flame");

			ImGui::SliderFloat("Corona", &settings.lightCoronaWidth, 0.0f, 30.0f, "%.2f studs", ImGuiSliderFlags_Logarithmic);
			tooltip("Width of the glow drawn at the light, 0 for none");

			ImGui::DragFloat3("Offset", &settings.lightOffset[0], 0.05f, -BrickAttachments::maxLightOffset, BrickAttachments::maxLightOffset, "%.2f");
			tooltip("Where the light is from the middle of the brick, in studs. Drag or double click to type");

			bool spotlight = settings.lightConeAngle > 0.0f;
			if (ImGui::Checkbox("Spotlight", &spotlight))
				settings.lightConeAngle = spotlight ? lastConeAngle : 0.0f;

			if (spotlight)
			{
				if (ImGui::SliderFloat("Cone angle", &settings.lightConeAngle, 1.0f, 179.0f, "%.0f degrees", ImGuiSliderFlags_AlwaysClamp))
					lastConeAngle = settings.lightConeAngle;
				tooltip("Full width of the beam");

				bool turned = ImGui::SliderFloat("Yaw", &lightYaw, -180.0f, 180.0f, "%.0f degrees");
				turned |= ImGui::SliderFloat("Pitch##Light", &lightPitch, -90.0f, 90.0f, "%.0f degrees");
				tooltip("-90 points straight down, 90 straight up");
				if (turned)
				{
					float yaw = glm::radians(lightYaw);
					float pitch = glm::radians(lightPitch);
					settings.lightDirection = glm::vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw));
				}

				ImGui::SliderFloat("Spin", &settings.lightSpin, -720.0f, 720.0f, "%.0f degrees/s");
				tooltip("Turns the beam around the vertical like a lighthouse. Ctrl+click to type up to 3600");
			}
		}
	}

	if (ImGui::CollapsingHeader("Emitter", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (emitterNames.empty())
			ImGui::TextDisabled("The server has no emitters");
		else
			nameCombo("Type##Emitter", settings.emitterName, emitterNames);
	}

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::Separator();

	if (ImGui::Button("Apply"))
	{
		submitted = true;
		close();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
		close();

	ImGui::End();
}

void WrenchDialog::init()
{

}

void WrenchDialog::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{

}

WrenchDialog::WrenchDialog()
{
	name = "Wrench Dialog";
}

WrenchDialog::~WrenchDialog()
{

}
