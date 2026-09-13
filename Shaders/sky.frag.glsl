#version 330 core

in vec3 viewRay;

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

//How far grass and water reach from the camera, see model.vert's cameraSpacePosition and LoopClient::waterRadius
const float surfaceRadius = 300.0;

void main()
{
	vec3 ray = normalize(viewRay);

	//Keep in sync with skyColorFor in water.frag
	vec3 sky = mix(FogColor, SkyColor, smoothstep(0.0, 0.4, ray.y));

	//From above the ground, the fogged far edge of the grass or water sits below straight ahead, with nothing drawn
	//between the two. The sun and moon need to set behind that edge, not straight ahead, or they vanish early
	float height = max(CameraPosition.y - HorizonHeight, 0.0);
	float visibleHorizon = -height / sqrt(height * height + surfaceRadius * surfaceRadius);

	if(ray.y > visibleHorizon)
	{
		float sunAmount = dot(ray, SunDirection);
		vec3 sunTint = mix(vec3(1.0, 0.45, 0.15), vec3(1.0, 0.95, 0.85), smoothstep(0.0, 0.3, SunDirection.y));
		sky += sunTint * pow(max(sunAmount, 0.0), 300.0) * 0.6;
		sky = mix(sky, sunTint, smoothstep(0.9990, 0.9995, sunAmount));

		float moonAmount = dot(ray, -SunDirection);
		sky = mix(sky, vec3(0.85, 0.88, 0.95), smoothstep(0.9994, 0.9997, moonAmount));
	}

	color = vec4(sky, 1.0);
}
