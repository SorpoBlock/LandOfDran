#include "DebugMenu.h"

void DebugMenu::passDetails(std::shared_ptr<Camera> camera)
{
	cameraPosition = camera->getPosition();
	cameraDirection = camera->getDirection();
}

void DebugMenu::render(ImGuiIO* io)
{
	if (!opened)
		return;

	if (!ImGui::Begin("Debug Menu", &opened))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginTabBar("debugTabs");

	if (ImGui::BeginTabItem("Performance"))
	{
		ImGui::Text("FPS: %f", io->Framerate);
		ImGui::EndTabItem();
	}

	if(ImGui::BeginTabItem("Camera"))
	{
		ImGui::Text("Camera position: %f,%f,%f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
		ImGui::Text("Camera direction: %f,%f,%f", cameraDirection.x, cameraDirection.y, cameraDirection.z);
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Console"))
	{
		ImGui::BeginGroup();
		float windowWidth = ImGui::GetContentRegionAvail().x;
		ImGui::BeginChild("debugScroll",ImVec2(windowWidth,200));
		ImGui::PushTextWrapPos(0.0f);

		const std::deque<loggerLine> * const lastLoggedLines = Logger::getStorage();
		for (int a = ((int)lastLoggedLines->size())-1; a >= 0; a--)
		{
			if (lastLoggedLines->at(a).isDebug && !showVerboseLogging)
				continue;

			if(lastLoggedLines->at(a).isError)
				ImGui::TextColored(ImVec4(1, 0, 0, 1), lastLoggedLines->at(a).text.c_str());
			else if(lastLoggedLines->at(a).isDebug)
				ImGui::TextColored(ImVec4(1,1,1,0.5), lastLoggedLines->at(a).text.c_str());
			else
			{
				if (lastLoggedLines->at(a).text.length() > 6)
				{
					if(lastLoggedLines->at(a).text.substr(0,6) == "INPUT ")
					{
						ImGui::TextColored(ImVec4(0, 1, 0, 1), lastLoggedLines->at(a).text.substr(6).c_str());
						continue;
					}
				}
				ImGui::Text(lastLoggedLines->at(a).text.c_str());
			}
		}

		if (scrollToBottom)
			ImGui::SetScrollY(ImGui::GetScrollMaxY());

		ImGui::PopTextWrapPos();
		ImGui::EndChild();
		ImGui::EndGroup();

		ImGui::Checkbox("Verbose", &showVerboseLogging);
		ImGui::SameLine();
		ImGui::Checkbox("Scroll lock", &scrollToBottom);
		
		if (ImGui::InputText("<- Lua", consoleCommandBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			std::string command = (consoleCommandBuffer);
			if (command.length() > 0)
			{
				info("INPUT " + command);
				error("Not implemented yet.");
				consoleCommandBuffer[0] = 0;
			}
		}

		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
	ImGui::End();
}

void DebugMenu::init()
{

}

void DebugMenu::handleInput(SDL_Event& e, std::shared_ptr<InputMap> input)
{

}

DebugMenu::DebugMenu()
{
	name = "Debug Menu";
	consoleCommandBuffer[0] = 0;
}

DebugMenu::~DebugMenu()
{

}
