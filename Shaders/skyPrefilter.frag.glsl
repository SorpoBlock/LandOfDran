#version 330 core

in vec2 facePosition;

out vec4 color;

//The clamped sky with plain mip levels, see Skybox::loadSlot
uniform samplerCube source;
//Size of the source's largest faces
uniform float sourceSize;
//0 to 1, how blurry this level's reflections are
uniform float roughness;
//Which cube map face this is, in OpenGL's order
uniform int face;

const float PI = 3.14159265359;
const uint SAMPLES = 256u;

//Keep in sync with cubeFaceDirection in Skybox.cpp
vec3 faceDirection(int index, float s, float t)
{
	if(index == 0) return vec3(1.0, -t, -s);
	if(index == 1) return vec3(-1.0, -t, s);
	if(index == 2) return vec3(s, 1.0, t);
	if(index == 3) return vec3(s, -1.0, -t);
	if(index == 4) return vec3(s, -t, 1.0);
	return vec3(-s, -t, -1.0);
}

float radicalInverse(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

//GGX blurred sky around N, seen straight on, the split sum approximation's first half
//Each sample reads a smaller mip level the more sky it stands for, which keeps the sun from sparkling, see
//Karis, Real Shading in Unreal Engine 4, and GPU Gems 3 chapter 20
void main()
{
	vec3 N = normalize(faceDirection(face, facePosition.x, facePosition.y));
	float a = roughness * roughness;

	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, N));
	vec3 tangentY = cross(N, tangentX);

	float texelSolidAngle = 4.0 * PI / (6.0 * sourceSize * sourceSize);

	vec3 total = vec3(0.0);
	float weight = 0.0;
	for(uint i = 0u; i < SAMPLES; i++)
	{
		vec2 Xi = vec2(float(i) / float(SAMPLES), radicalInverse(i));
		float phi = 2.0 * PI * Xi.x;
		float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
		float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
		vec3 H = tangentX * cos(phi) * sinTheta + tangentY * sin(phi) * sinTheta + N * cosTheta;
		vec3 L = 2.0 * dot(N, H) * H - N;

		float NdotL = dot(N, L);
		if(NdotL <= 0.0)
			continue;

		//With the view along N, NdotH and VdotH are both cosTheta, so the sample's pdf is just D / 4
		float denominator = cosTheta * cosTheta * (a * a - 1.0) + 1.0;
		float D = a * a / (PI * denominator * denominator);
		float sampleSolidAngle = 1.0 / (float(SAMPLES) * D * 0.25 + 0.0001);
		float level = max(0.5 * log2(sampleSolidAngle / texelSolidAngle) + 1.0, 0.0);

		total += textureLod(source, L, level).rgb * NdotL;
		weight += NdotL;
	}

	color = vec4(total / max(weight, 0.0001), 1.0);
}
