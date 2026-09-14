#version 330 core

in vec2 uv;

out vec4 color;

layout (std140) uniform EnvironmentUniforms
{
	//See EnvironmentUniforms in ShaderSpecification.h
	vec3 SunDirection;
	float FogDistanceMin;
	vec3 LightDirection;
	float FogDistanceMax;
	vec3 LightColor;
	float WaveTime;
	vec3 SkyColor;
	float WaterLevel;
	vec3 FogColor;
	float HorizonHeight;
	vec4 ClipPlane;
	vec3 AmbientColor;
	float ShadowStrength;
};

const float TAU = 6.28318530718;

//Drawn over the whole screen while the camera is under the water
void main()
{
	//WaveTime wraps every 100 seconds, so each wave does a whole number of cycles in that time and loops without a jump
	float cycle = WaveTime * TAU / 100.0;
	float waves = sin(uv.x * 9.0 + uv.y * 4.0 + cycle * 23.0)
		+ sin(uv.x * -6.0 + uv.y * 11.0 + cycle * 31.0)
		+ sin((uv.x + uv.y) * 17.0 - cycle * 41.0) * 0.5;
	waves = waves / 5.0 + 0.5;

	//Deeper blue toward the edges of the screen, with an edge that slowly wobbles
	//Whole multiples of the angle, so the wobble meets itself where atan wraps around
	vec2 centered = uv - 0.5;
	float angle = atan(centered.y, centered.x);
	float wobble = sin(angle * 5.0 + cycle * 29.0) * 0.025 + sin(angle * 9.0 - cycle * 37.0) * 0.015 + (waves - 0.5) * 0.04;
	float vignette = smoothstep(0.2, 0.75, length(centered) + wobble);

	//Darker at night, roughly following how bright the horizon is, like water.frag
	float brightness = clamp(dot(FogColor, vec3(0.333)) * 1.3, 0.05, 1.0);
	vec3 tint = mix(vec3(0.08, 0.38, 0.6), vec3(0.02, 0.2, 0.55), vignette) * brightness;

	color = vec4(tint, 0.15 + waves * 0.06 + vignette * (0.4 + waves * 0.1));
}
