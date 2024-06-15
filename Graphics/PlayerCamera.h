#pragma once

#include "../LandOfDran.h"

#include "ShaderSpecification.h"

class Camera
{
	glm::mat4 viewMatrix = glm::mat4(1.0);

	//Like the view matrix, but only rotation, not translation, good for rendering skyboxes and stuff
	glm::mat4 angleMatrix = glm::mat4(1.0);

	glm::mat4 projectionMatrix = glm::mat4(1.0);

	//In world space
	glm::vec3 position = glm::vec3(0, 0, 0);

	//What direction the camera is pointing
	glm::vec3 direction = glm::vec3(0, 0, 1);

	//Should almost always remain 0,1,0 (up) but we could implement camera shake, rag doll camera, etc.
	glm::vec3 nominalUp = glm::vec3(0, 1, 0);

	float fieldOfVision = 90.0;
	float nearPlane = 0.5;
	float farPlane = 1000.0;
	float aspectRatio = 1.0;

	public:

	float mouseSensitivity = 1.0;
	bool invertMouse = false;

	//Positive amount forward, negative backward, only use for no-clip camera
	void flyStraight(float amount);

	//Positive amount right, negative left, only use for no-clip camera
	void flySideways(float amount);

	//Look around with the mouse
	void turn(float relMouseX, float relMouseY);

	//Call once per frame
	void render(ShaderManager * graphics);

	void setFOV(float fov);

	//Call when screen size changed
	void setAspectRatio(float ratio);

	Camera(float _aspectRatio = 1.0, float _fieldOfVision = 90.0, float _nearPlane = 0.5, float _farPlane = 1000.0);
};