#include "Environment.h"

#include <glm/gtc/constants.hpp>

void Environment::calc(double worldTimeSeconds)
{
	dayFraction = (float)(fmod(worldTimeSeconds, DAY_LENGTH_SECONDS) / DAY_LENGTH_SECONDS);
	if (dayFraction < 0)
		dayFraction += 1.0f;

	//Rises in +X and sets in -X, tilted toward +Z so it's never directly overhead
	const float tilt = glm::radians(20.0f);
	float angle = (dayFraction - 0.25f) * glm::two_pi<float>();
	sunDirection = glm::normalize(glm::vec3(cos(angle), sin(angle) * cos(tilt), sin(angle) * sin(tilt)));

	float elevation = sunDirection.y;
	const SkyKeyframe& twilight = dayFraction < 0.5f ? dawn : dusk;

	auto blend = [](const SkyKeyframe& a, const SkyKeyframe& b, float t) -> SkyKeyframe
	{
		return { glm::mix(a.skyColor, b.skyColor, t), glm::mix(a.fogColor, b.fogColor, t), glm::mix(a.lightColor, b.lightColor, t), glm::mix(a.ambientColor, b.ambientColor, t) };
	};

	SkyKeyframe current;
	if (elevation < 0.05f)
		current = blend(night, twilight, glm::smoothstep(-0.25f, 0.05f, elevation));
	else
		current = blend(twilight, day, glm::smoothstep(0.05f, 0.35f, elevation));

	skyColor = current.skyColor;
	fogColor = current.fogColor;
	ambientColor = current.ambientColor;

	//Direct light fades to nothing right at the horizon so switching between sun and moon doesn't pop
	if (elevation >= 0)
	{
		lightDirection = sunDirection;
		shadowStrength = glm::smoothstep(0.0f, 0.08f, elevation);
		lightColor = current.lightColor * shadowStrength;
	}
	else
	{
		lightDirection = -sunDirection;
		shadowStrength = glm::smoothstep(0.0f, 0.08f, -elevation);
		lightColor = night.lightColor * shadowStrength;
	}
}

void Environment::passUniforms(std::shared_ptr<ShaderManager> shaders) const
{
	EnvironmentUniforms& uniforms = shaders->environmentUniforms;
	uniforms.SunDirection = sunDirection;
	uniforms.LightDirection = lightDirection;
	uniforms.LightColor = lightColor;
	uniforms.SkyColor = skyColor;
	uniforms.FogColor = fogColor;
	uniforms.FogDistanceMin = fogDistanceMin;
	uniforms.FogDistanceMax = fogDistanceMax;
	uniforms.AmbientColor = ambientColor;
	uniforms.ShadowStrength = shadowStrength;
}

Environment::Environment()
{
	//Sky and fog colors are final screen colors, light and ambient colors are HDR (tone mapped in model.frag)
	//Night's light color is the moon. Day's ambient matches the ambient model.frag had before the day/night cycle
	night = { glm::vec3(0.01, 0.015, 0.04),	glm::vec3(0.03, 0.04, 0.08),	glm::vec3(0.5, 0.6, 1.0) * 0.35f,	glm::vec3(0.015, 0.018, 0.03) };
	dawn =	{ glm::vec3(0.25, 0.3, 0.5),	glm::vec3(0.95, 0.6, 0.35),		glm::vec3(1.0, 0.55, 0.25) * 6.0f,	glm::vec3(0.25, 0.2, 0.22) };
	day =	{ glm::vec3(0.25, 0.45, 0.85),	glm::vec3(0.65, 0.78, 0.92),	glm::vec3(1.0, 0.7, 0.5) * 15.0f,	glm::vec3(1.0, 0.7, 0.5) * 0.45f };
	dusk =	{ glm::vec3(0.22, 0.22, 0.45),	glm::vec3(0.9, 0.45, 0.25),		glm::vec3(1.0, 0.45, 0.2) * 6.0f,	glm::vec3(0.24, 0.17, 0.18) };

	calc(DAY_LENGTH_SECONDS * 0.5);
}
