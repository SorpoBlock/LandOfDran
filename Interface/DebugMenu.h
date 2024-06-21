#pragma once

#include "../LandOfDran.h"

#include "UserInterface.h"
#include "../Graphics/PlayerCamera.h"

class DebugMenu : public Window
{
	friend class Window;
	friend class UserInterface;

	glm::vec3 cameraPosition = glm::vec3(0,0,0);
	glm::vec3 cameraDirection = glm::vec3(0,0,0);

	virtual void render(ImGuiIO* io) override;
	virtual void init() override;

	bool showVerboseLogging = false;
	bool scrollToBottom = true;

	char consoleCommandBuffer[256];

	DebugMenu();

public:

	//Pass info to the debug menu to render and display later on
	void passDetails(std::shared_ptr<Camera> camera);

	~DebugMenu();
};




