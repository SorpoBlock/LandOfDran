#version 330 core

in vec3 worldPos;
in vec4 clipSpace;

out vec4 color;

layout (std140) uniform CameraUniforms
{
	//Camera Uniforms:
	mat4 CameraProjection;
	mat4 CameraView;
	mat4 CameraAngle;
	vec3 CameraPosition;
	vec3 CameraDirection;
};

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

uniform sampler2D Reflection;
uniform sampler2D Refraction;

//Off when graphics/waterquality is off, reflection is also off while the camera is underwater
uniform bool useReflection;
uniform bool useRefraction;
uniform bool cameraUnderwater;

const float TAU = 6.28318530718;

//The first three are the waves water.vert displaces the mesh with, the rest are ripples only used for shading
const int WAVES = 6;
const vec3 waves[WAVES] = vec3[](
	vec3(0.21, 0.13, 0.12),
	vec3(-0.17, 0.29, 0.08),
	vec3(0.53, -0.41, 0.04),
	vec3(1.3, 0.9, 0.02),
	vec3(-2.1, 1.7, 0.012),
	vec3(3.1, 2.9, 0.006));
const float waveCycles[WAVES] = float[](11.0, 17.0, 31.0, 43.0, 59.0, 73.0);

vec3 waveNormal(vec2 p)
{
	vec2 slope = vec2(0.0);
	for(int i = 0; i < WAVES; i++)
		slope += waves[i].z * waves[i].xy * cos(dot(p, waves[i].xy) + WaveTime * TAU * waveCycles[i] / 100.0);
	return normalize(vec3(-slope.x, 1.0, -slope.y));
}

//Same gradient as sky.frag, minus the sun and moon
vec3 skyColorFor(vec3 ray)
{
	return mix(FogColor, SkyColor, smoothstep(0.0, 0.4, ray.y));
}

void main()
{
	vec3 toCamera = CameraPosition - worldPos;
	float distanceToCamera = length(toCamera);
	toCamera /= distanceToCamera;

	//Far away ripples are smaller than a pixel and just shimmer
	vec3 normal = normalize(mix(waveNormal(worldPos.xz), vec3(0, 1, 0), clamp(distanceToCamera / 150.0, 0.0, 1.0)));
	vec3 facingNormal = cameraUnderwater ? -normal : normal;

	vec2 screenUV = (clipSpace.xy / clipSpace.w) * 0.5 + 0.5;
	vec2 distortion = normal.xz * 0.05;

	//Darker at night, roughly following how bright the horizon is
	vec3 waterTint = vec3(0.1, 0.3, 0.35) * clamp(dot(FogColor, vec3(0.333)) * 1.3, 0.05, 1.0);

	vec3 refraction = waterTint;
	if(useRefraction)
		refraction = mix(texture(Refraction, clamp(screenUV + distortion, 0.001, 0.999)).rgb, waterTint, 0.3);

	//The reflection camera keeps a normal up vector, so its image is upside down compared to a real reflection
	vec3 reflection;
	if(useReflection)
		reflection = texture(Reflection, clamp(vec2(screenUV.x, 1.0 - screenUV.y) + distortion, 0.001, 0.999)).rgb;
	else
		reflection = skyColorFor(reflect(-toCamera, normal));

	//See-through looking straight down, reflective at grazing angles, never reflective from underneath
	float fresnel = cameraUnderwater ? 0.0 : pow(1.0 - max(dot(toCamera, facingNormal), 0.0), 3.0);

	vec3 halfVector = normalize(LightDirection + toCamera);
	float specular = cameraUnderwater ? 0.0 : pow(max(dot(normal, halfVector), 0.0), 300.0);

	vec3 result = mix(refraction, reflection, fresnel) + LightColor * specular * 0.1;

	float fogFactor = clamp((distanceToCamera - FogDistanceMin) / (FogDistanceMax - FogDistanceMin), 0.0, 1.0);
	color = vec4(mix(result, FogColor, fogFactor), 1.0);
}
