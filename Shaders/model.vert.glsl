#version 330 core

layout(location = 0) in vec3 ModelSpace;
layout(location = 1) in vec3 NormalVector;
layout(location = 2) in vec3 TangentVector;
layout(location = 3) in vec3 BitangentVector;
layout(location = 4) in vec2 TextureCoords;
layout(location = 5) in vec4 PreColor;
layout(location = 6) in int  InstanceFlags;
layout(location = 7) in mat4 ModelTransform;

layout (std140) uniform BasicUniforms
{
	//Model Matrix:
	mat4 TranslationMatrix;
	mat4 RotationMatrix;
	mat4 ScaleMatrix;
	
	//Material uniforms:
	//These are -1 if not used, otherwise they point to what layer of their 2d texture array they are on
	int useAlbedo;
	int useNormal;
	int useMetalness;
	int useRoughness;
	int useHeight;
	int useAO;
	
	bool nonInstanced;
	bool cameraSpacePosition;
};

layout (std140) uniform CameraUniforms
{
	//Camera Uniforms:
	mat4 CameraProjection;
	mat4 CameraView;
	mat4 CameraAngle;
	vec3 CameraPosition;
	vec3 CameraDirection;
};

out vec2 uvs;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec3 worldPos;
out vec4 preColor;
flat out int  useDecal;

void main()
{
	preColor = PreColor;
	useDecal = (InstanceFlags & 131072) == 131072 ? ((InstanceFlags & 130560) >> 9) : -1;
	uvs = TextureCoords;
	
	mat4 transform;
	if(nonInstanced)
	{
		preColor = vec4(0,0,0,0);
		transform = TranslationMatrix * RotationMatrix * ScaleMatrix;
	}
	else
		transform = ModelTransform;
	
	worldPos = (transform * vec4(ModelSpace,1)).xyz;
		
	if(cameraSpacePosition)
	{
		worldPos = vec3(CameraPosition.x+ModelSpace.x*300.0,0,CameraPosition.z+ModelSpace.z*300.0);
		uvs = worldPos.xz / 20.0;
	}
	
	normal = (transform * vec4(NormalVector,0)).xyz;
	tangent = (transform * vec4(TangentVector,0)).xyz;
	bitangent = (transform * vec4(BitangentVector,0)).xyz;
	
	gl_Position = CameraProjection * CameraView * vec4(worldPos,1);
}

