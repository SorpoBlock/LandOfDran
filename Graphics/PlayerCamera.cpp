#include "PlayerCamera.h"

void Camera::updateSettings(std::shared_ptr<SettingManager> settings)
{
    mouseSensitivity = settings->getFloat("input/mousesensitivity");
    invertMouse = settings->getBool("input/invertmousey");
}

glm::vec3 Camera::getPosition()
{
    return position;
}

glm::vec3 Camera::getDirection()
{
    return direction;
}

void Camera::setFOV(float fov)
{
    fieldOfVision = fov;
    projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
}

//Call when screen size changed
void Camera::setAspectRatio(float ratio)
{
    aspectRatio = ratio;
    projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
}

Camera::Camera(float _aspectRatio, float _fieldOfVision, float _nearPlane, float _farPlane)
    : aspectRatio(_aspectRatio), fieldOfVision(_fieldOfVision), nearPlane(_nearPlane), farPlane(_farPlane)
{
    projectionMatrix = glm::perspective(glm::radians(fieldOfVision), aspectRatio, nearPlane, farPlane);
}

//Positive amount forward, negative backward, only use for no-clip camera
void Camera::flyStraight(float amount)
{
	position = position + glm::vec3(direction.x * amount, direction.y * amount, direction.z * amount);
}

//Positive amount right, negative left, only use for no-clip camera
void Camera::flySideways(float amount)
{
    float y = atan2(direction.x, direction.z);
    y += 1.57079633f;
    if (y > 6.28318531f)
        y -= 6.28318531f;

    position = position + glm::vec3(sin(y) * amount, 0, cos(y) * amount);
}

//Look around with the mouse
void Camera::turn(float relMouseX, float relMouseY)
{
    relMouseX *= mouseSensitivity / 100.0f;
    relMouseY *= mouseSensitivity / 100.0f;
    if (invertMouse)
        relMouseY = -relMouseY;

    if (isnan(direction.y))
        direction.y = 0;
    if (isnan(direction.x))
        direction.x = 0;
    if (isnan(direction.z))
        direction.z = 0;

    float p = asin(direction.y);
    float y = atan2(direction.x, direction.z);
    float newP = p + relMouseY;
    if (newP >= 1.57f)
        newP = 1.57f;
    if (newP <= -1.57f)
        newP = -1.57f;

    direction = glm::vec3(cos(newP) * sin(y + relMouseX), sin(newP), cos(newP) * cos(y + relMouseX));
}

//Call once per frame
void Camera::render(ShaderManager* graphics)
{
    viewMatrix = glm::lookAt(position, position + direction, nominalUp);
    angleMatrix = glm::lookAt(glm::vec3(0, 0, 0), direction, nominalUp);

    graphics->cameraUniforms.CameraDirection = direction;
    graphics->cameraUniforms.CameraPosition = position;
    graphics->cameraUniforms.CameraProjection = projectionMatrix;
    graphics->cameraUniforms.CameraView = viewMatrix;
    graphics->cameraUniforms.CameraAngle = angleMatrix;

    graphics->updateCameraUBO();
}
