#include "PlayerCamera.h"

void Camera::control(float deltaT,std::shared_ptr<InputMap> input)
{
    if (!(freePosition && target.expired()))
        return;

    //Test camera controls, no-clip camera
    float speed = 0.015f;

    if (input->isCommandKeydown(WalkForward))
        flyStraight(deltaT * speed);
    if (input->isCommandKeydown(WalkBackward))
        flyStraight(-deltaT * speed);
    if (input->isCommandKeydown(WalkRight))
        flySideways(-deltaT * speed);
    if (input->isCommandKeydown(WalkLeft))
        flySideways(deltaT * speed);
}

void Camera::updateSettings(std::shared_ptr<SettingManager> settings)
{
    mouseSensitivity = settings->getFloat("input/mousesensitivity");
    invertMouse = settings->getBool("input/invertmousey");
}

void Camera::setPosition(const glm::vec3& pos)
{
    position = pos;
}

void Camera::setDirection(const glm::vec3& dir)
{
    direction = dir;

    //If either of the coords are exactly 0 the shader will stop rendering anything
    if (std::abs(direction.x) < 0.001)
        direction.x = 0.001;
    if (std::abs(direction.y) < 0.001)
        direction.y = 0.001;
    if (std::abs(direction.z) < 0.001)
        direction.z = 0.001;
}

void Camera::setUp(const glm::vec3& up)
{
	nominalUp = up;
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
    if (!freeDirection)
        return;

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
void Camera::render(std::shared_ptr<ShaderManager> graphics)
{
    if(!target.expired())
	{
        std::shared_ptr<Dynamic> targetLock = target.lock();
        if (targetLock->clientControlled)
        {
            btTransform t = targetLock->body->getWorldTransform();
            position = b2g3(t.getOrigin());

            glm::vec3 eyePos = targetLock->getType()->getModel()->getEyePosition();
            glm::vec4 homoEyePos = glm::toMat4(targetLock->interpolator.getRotation()) * glm::vec4(eyePos, 1);
            position += glm::vec3(homoEyePos.x, homoEyePos.y, homoEyePos.z);

            const btQuaternion &q = t.getRotation();
            glm::quat quat = glm::quat(q.getW(), q.getX(), q.getY(), q.getZ());
            glm::vec4 homoDir = glm::toMat4(quat) * glm::vec4(0, 0, 1, 0);
            if (!freeDirection)
                direction = glm::vec3(homoDir.x, homoDir.y, homoDir.z);
            if(freeUpVector)
			{
				glm::vec4 homoUp = glm::toMat4(quat) * glm::vec4(0, 1, 0, 0);
				nominalUp = glm::vec3(homoUp.x, homoUp.y, homoUp.z);
			}
            else
                nominalUp = glm::vec3(0, 1, 0);
        }
        else
        {
            position = targetLock->interpolator.getPosition();

            glm::vec3 eyePos = targetLock->getType()->getModel()->getEyePosition();
            glm::vec4 homoEyePos = glm::toMat4(targetLock->interpolator.getRotation()) * glm::vec4(eyePos, 1);
            position += glm::vec3(homoEyePos.x, homoEyePos.y, homoEyePos.z);

            glm::vec4 homoDir = glm::toMat4(targetLock->interpolator.getRotation()) * glm::vec4(0, 0, 1, 0);
            if(!freeDirection)
                direction = glm::vec3(homoDir.x, homoDir.y, homoDir.z);
            if(freeUpVector)
            {
                glm::vec4 homoUp = glm::toMat4(targetLock->interpolator.getRotation()) * glm::vec4(0, 1, 0, 0);
				nominalUp = glm::vec3(homoUp.x, homoUp.y, homoUp.z);
			}
            else
                nominalUp = glm::vec3(0, 1, 0);
        }
	}

    viewMatrix = glm::lookAt(position, position + direction, nominalUp);
    angleMatrix = glm::lookAt(glm::vec3(0, 0, 0), direction, nominalUp);

    graphics->cameraUniforms.CameraDirection = direction;
    graphics->cameraUniforms.CameraPosition = position;
    graphics->cameraUniforms.CameraProjection = projectionMatrix;
    graphics->cameraUniforms.CameraView = viewMatrix;
    graphics->cameraUniforms.CameraAngle = angleMatrix;

    graphics->updateCameraUBO();
}
