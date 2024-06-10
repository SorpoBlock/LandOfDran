#version 330 core

layout(location = 0) in vec3 positions;
layout(location = 1) in vec2 uv;

layout (std140) uniform GlobalUniforms
{
	//Model Matrix:
	mat4 TranslationMatrix;
	mat4 RotationMatrix;
	mat4 ScaleMatrix;
	
	//Camera Uniforms:
	mat4 CameraProjection;
	mat4 CameraView;
	vec3 CameraPosition;
	vec3 CameraDirection;
	
	//Material uniforms:
	int useAlbedo;
	int useNormal;
	int useMetalness;
	int useRoughness;
	int useHeight;
	int useAO;
};

out vec2 uvs;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec3 worldPos;

void main()
{
	uvs = uv;
	worldPos = (TranslationMatrix * RotationMatrix * ScaleMatrix * vec4(positions,1)).xyz;
	normal = (RotationMatrix * vec4(0,0,1,0)).xyz;
	gl_Position = CameraProjection * CameraView * vec4(worldPos,1);
}

