#version 330 core

//The shared cube, see makeCube in Graphics/InstancedBrickRenderer.cpp
layout(location = 0) in vec3 CubePosition;
layout(location = 1) in vec3 CubeNormal;
layout(location = 2) in vec3 CubeTangent;
layout(location = 3) in vec3 CubeBitangent;
layout(location = 4) in vec2 CubeUV;

//Per brick, in studs horizontally and plates vertically, with width and length already swapped for rotation
layout(location = 5) in vec3 BrickCorner;
layout(location = 6) in vec3 BrickSize;
layout(location = 7) in vec4 BrickColor;

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

uniform mat4 lightSpaceMatricies[3];

//Top and bottom faces repeat their texture once per stud, side faces stretch it once across the whole face
uniform bool tileByStuds;

//Identity for placed bricks, rotation and position for loose bricks like undo debris
uniform mat4 brickTransform;

//STUD_SIZE and PLATE_SIZE in Bricks/Brick.h
const vec3 gridScale = vec3(1.0, 0.4, 1.0);

out vec2 uvs;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec3 worldPos;
out vec4 preColor;
out float opacity;
flat out int useDecal;
out vec4 shadowPos[3];

void main()
{
	worldPos = (brickTransform * vec4((BrickCorner + CubePosition * BrickSize) * gridScale, 1.0)).xyz;

	mat3 rotation = mat3(brickTransform);
	normal = rotation * CubeNormal;
	tangent = rotation * CubeTangent;
	bitangent = rotation * CubeBitangent;

	//Tiled faces are the top and bottom, whose texture axes run along x and z
	uvs = tileByStuds ? CubeUV * BrickSize.xz : CubeUV;

	preColor = vec4(BrickColor.rgb, 1.0);
	opacity = BrickColor.a;
	useDecal = -1;

	for(int i = 0; i<3; i++)
		shadowPos[i] = lightSpaceMatricies[i] * vec4(worldPos,1.0);

	gl_ClipDistance[0] = dot(vec4(worldPos, 1.0), ClipPlane);
	gl_Position = CameraProjection * CameraView * vec4(worldPos,1.0);
}
