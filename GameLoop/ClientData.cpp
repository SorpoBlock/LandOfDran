#include "ClientData.h"

#include "ServerProgramData.h"
#include "../LuaFunctions/SoundLua.h"

//A bright, wide spotlight
static constexpr float flashlightBrightness = 150.0f;
static constexpr float flashlightConeAngle = 80.0f;

void ClientData::setFlashlight(const ServerProgramData* pd, bool on, const glm::vec3& color)
{
	if (!pd->lights)
		return;

	std::shared_ptr<Light> light = flashlight.lock();
	std::shared_ptr<Dynamic> holder = controllers.empty() ? nullptr : controllers[0].target.lock();

	if (on && flashlightEnabled && holder)
	{
		if (light)
		{
			if (light->getColor() != glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f)))
				light->setColor(color);
			return;
		}

		light = pd->lights->create(b2g3(holder->getPosition()), color, flashlightBrightness, 0.0f, 0.0f);
		light->setConeAngle(flashlightConeAngle);
		light->setDirection(controllers[0].lastCameraDirection);
		light->setHolder(holder);
		flashlight = light;

		playSoundOn("LightOn", holder, 1.0f, 1.0f);
		return;
	}

	if (!light)
		return;

	glm::vec3 lastPosition = light->getPosition();
	pd->lights->destroy(light);
	flashlight.reset();

	if (holder)
		playSoundOn("LightOff", holder, 1.0f, 1.0f);
	else
		playSoundAt("LightOff", lastPosition, 1.0f, 1.0f);
}

void ClientData::sendAbilities() const
{
	if (!client)
		return;

	char data[2] = { (char)PlayerAbilities, (char)((jetsEnabled ? PlayerAbility_Jets : 0) | (flashlightEnabled ? PlayerAbility_Flashlight : 0)) };
	client->send(data, 2, OtherReliable);
}

void ClientData::removeEffects(const ServerProgramData* pd)
{
	if (std::shared_ptr<Light> light = flashlight.lock())
		pd->lights->destroy(light);
	flashlight.reset();

	for (PlayerController& controller : controllers)
	{
		for (std::weak_ptr<Emitter>& jet : controller.jetEmitters)
		{
			if (std::shared_ptr<Emitter> flame = jet.lock())
				pd->emitters->destroy(flame);
			jet.reset();
		}
	}
}
